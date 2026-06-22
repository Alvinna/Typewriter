#!/bin/sh

export LC_ALL="en_US.UTF-8"

# Compute our working directory in an extremely defensive manner
SCRIPT_DIR="$(CDPATH='' cd -- "$(dirname -- "$0")" && pwd -P)"
# NOTE: We need to remember the *actual* KOREADER_DIR, not the relocalized version in /tmp...
export KOREADER_DIR="${KOREADER_DIR:-${SCRIPT_DIR}}"
UNPACK_DIR="${KOREADER_DIR%/*}"

# We rely on starting from our working directory, and it needs to be set, sane and absolute.
cd "${KOREADER_DIR:-/dev/null}" || exit

if [ "${SCRIPT_DIR}" != "/tmp" ]; then
    cp -pf "${0}" "/tmp/typewriter.sh"
    chmod 777 "/tmp/typewriter.sh"
    exec "/tmp/typewriter.sh" "$@"
fi

# Attempt to switch to a sensible CPUFreq governor when that's not already the case...
# Swap every CPU at once if available
if [ -d "/sys/devices/system/cpu/cpufreq/policy0" ]; then
    CPUFREQ_SYSFS_PATH="/sys/devices/system/cpu/cpufreq/policy0"
else
    CPUFREQ_SYSFS_PATH="/sys/devices/system/cpu/cpu0/cpufreq"
fi
IFS= read -r current_cpufreq_gov <"${CPUFREQ_SYSFS_PATH}/scaling_governor"
# NOTE: What's available depends on the HW, so, we'll have to take it step by step...
#       Roughly follow Nickel's behavior (which prefers interactive), and prefer interactive, then ondemand, and finally conservative/dvfs.
if [ "${current_cpufreq_gov}" != "interactive" ]; then
    if grep -q "interactive" "${CPUFREQ_SYSFS_PATH}/scaling_available_governors"; then
        ORIG_CPUFREQ_GOV="${current_cpufreq_gov}"
        echo "interactive" >"${CPUFREQ_SYSFS_PATH}/scaling_governor"
    elif [ "${current_cpufreq_gov}" != "ondemand" ]; then
        if grep -q "ondemand" "${CPUFREQ_SYSFS_PATH}/scaling_available_governors"; then
            # NOTE: This should never really happen: every kernel that supports ondemand already supports interactive ;).
            #       They were both introduced on Mk. 6
            ORIG_CPUFREQ_GOV="${current_cpufreq_gov}"
            echo "ondemand" >"${CPUFREQ_SYSFS_PATH}/scaling_governor"
        elif [ -e "/sys/devices/platform/mxc_dvfs_core.0/enable" ]; then
            # The rest of this block assumes userspace is available...
            if grep -q "userspace" "${CPUFREQ_SYSFS_PATH}/scaling_available_governors"; then
                ORIG_CPUFREQ_GOV="${current_cpufreq_gov}"
                export CPUFREQ_DVFS="true"

                # If we can use conservative, do so, but we'll tweak it a bit to make it somewhat useful given our load patterns...
                # We unfortunately don't have any better choices on those kernels,
                # the only other governors available are powersave & performance (c.f., #4114)...
                if grep -q "conservative" "${CPUFREQ_SYSFS_PATH}/scaling_available_governors"; then
                    export CPUFREQ_CONSERVATIVE="true"
                    echo "conservative" >"${CPUFREQ_SYSFS_PATH}/scaling_governor"
                    # NOTE: The knobs survive a governor switch, which is why we do this now ;).
                    echo "2" >"/sys/devices/system/cpu/cpufreq/conservative/sampling_down_factor"
                    echo "50" >"/sys/devices/system/cpu/cpufreq/conservative/freq_step"
                    echo "11" >"/sys/devices/system/cpu/cpufreq/conservative/down_threshold"
                    echo "12" >"/sys/devices/system/cpu/cpufreq/conservative/up_threshold"
                    # NOTE: The default sampling_rate is a bit high for my tastes,
                    #       but it unfortunately defaults to its lowest possible setting...
                fi

                # NOTE: Now, here comes the freaky stuff... On a H2O, DVFS is only enabled when Wi-Fi is *on*.
                #       When it's off, DVFS is off, which pegs the CPU @ max clock given that DVFS means the userspace governor.
                #       The flip may originally have been switched by the sdio_wifi_pwr module itself,
                #       via ntx_wifi_power_ctrl @ arch/arm/mach-mx5/mx50_ntx_io.c (which is also the CM_WIFI_CTRL (208) ntx_io ioctl),
                #       but the code in the published H2O kernel sources actually does the reverse, and is commented out ;).
                #       It is now entirely handled by Nickel, right *before* loading/unloading that module.
                #       (There's also a bug(?) where that behavior is inverted for the *first* Wi-Fi session after a cold boot...)
                if grep -q "^sdio_wifi_pwr " "/proc/modules"; then
                    # Wi-Fi is enabled, make sure DVFS is on
                    echo "userspace" >"${CPUFREQ_SYSFS_PATH}/scaling_governor"
                    echo "1" >"/sys/devices/platform/mxc_dvfs_core.0/enable"
                else
                    # Wi-Fi is disabled, make sure DVFS is off
                    echo "0" >"/sys/devices/platform/mxc_dvfs_core.0/enable"

                    # Switch to conservative to avoid being stuck at max clock if we can...
                    if [ -n "${CPUFREQ_CONSERVATIVE}" ]; then
                        echo "conservative" >"${CPUFREQ_SYSFS_PATH}/scaling_governor"
                    else
                        # Otherwise, we'll be pegged at max clock...
                        echo "userspace" >"${CPUFREQ_SYSFS_PATH}/scaling_governor"
                        # The kernel should already be taking care of that...
                        cat "${CPUFREQ_SYSFS_PATH}/scaling_max_freq" >"${CPUFREQ_SYSFS_PATH}/scaling_setspeed"
                    fi
                fi
            fi
        fi
    fi
fi


# export external font directory
export EXT_FONT_DIR="/mnt/onboard/fonts"

# Quick'n dirty way of checking if we were started while Nickel was running (e.g., KFMon),
# or from another launcher entirely, outside of Nickel (e.g., KSM).
VIA_NICKEL="false"
if pkill -0 nickel; then
    VIA_NICKEL="true"
fi
# NOTE: Do not delete this line because KSM detects newer versions of KOReader by the presence of the phrase 'from_nickel'.

if [ "${VIA_NICKEL}" = "true" ]; then
    # Detect if we were started from KFMon
    FROM_KFMON="false"
    if pkill -0 kfmon; then
        # That's a start, now check if KFMon truly is our parent...
        if [ "$(pidof -s kfmon)" -eq "${PPID}" ]; then
            FROM_KFMON="true"
        fi
    fi

    # Check if Nickel is our parent...
    FROM_NICKEL="false"
    if [ -n "${NICKEL_HOME}" ]; then
        FROM_NICKEL="true"
    fi

    # If we were spawned outside of Nickel, we'll need a few extra bits from its own env...
    if [ "${FROM_NICKEL}" = "false" ]; then
        # Siphon a few things from nickel's env (namely, stuff exported by rcS *after* on-animator.sh has been launched)...
        # shellcheck disable=SC2046
        export $(grep -s -E -e '^(DBUS_SESSION_BUS_ADDRESS|NICKEL_HOME|WIFI_MODULE|LANG|INTERFACE)=' "/proc/$(pidof -s nickel)/environ")
        # NOTE: Quoted variant, w/ the busybox RS quirk (c.f., https://unix.stackexchange.com/a/125146):
        #eval "$(awk -v 'RS="\0"' '/^(DBUS_SESSION_BUS_ADDRESS|NICKEL_HOME|WIFI_MODULE|LANG|INTERFACE)=/{gsub("\047", "\047\\\047\047"); print "export \047" $0 "\047"}' "/proc/$(pidof -s nickel)/environ")"
    fi

    # If bluetooth is enabled, kill it.
    if [ -e "/sys/devices/platform/bt/rfkill/rfkill0/state" ]; then
        # That's on sunxi, at least
        IFS= read -r bt_state <"/sys/devices/platform/bt/rfkill/rfkill0/state"
        if [ "${bt_state}" = "1" ]; then
            echo "0" >"/sys/devices/platform/bt/rfkill/rfkill0/state"

            # Power the chip down
            ./luajit frontend/device/kobo/ntx_io.lua 126 0
        fi
    fi
    if grep -q "^sdio_bt_pwr " "/proc/modules"; then
        # And that's on NXP SoCs
        rmmod sdio_bt_pwr
    fi

    # Flush disks, might help avoid trashing nickel's DB...
    sync
    # And we can now stop the full Kobo software stack
    # NOTE: We don't need to kill KFMon, it's smart enough not to allow running anything else while we're up
    # NOTE: We kill Nickel's master dhcpcd daemon on purpose,
    #       as we want to be able to use our own per-if processes w/ custom args later on.
    #       A SIGTERM does not break anything, it'll just prevent automatic lease renewal until the time
    #       KOReader actually sets the if up itself (i.e., it'll do)...
    killall -q -TERM nickel hindenburg sickel fickel strickel fontickel adobehost foxitpdf iink dhcpcd-dbus dhcpcd bluealsa bluetoothd fmon nanoclock.lua

    # Wait for Nickel to die... (oh, procps with killall -w, how I miss you...)
    kill_timeout=0
    while pkill -0 nickel; do
        # Stop waiting after 4s
        if [ ${kill_timeout} -ge 15 ]; then
            break
        fi
        usleep 250000
        kill_timeout=$((kill_timeout + 1))
    done
    # Remove Nickel's FIFO to avoid udev & udhcpc scripts hanging on open() on it...
    rm -f /tmp/nickel-hardware-status

    # We don't need to grab input devices (unless MiniClock is running, in which case that neatly inhibits it while we run).
    if [ ! -d "/tmp/MiniClock" ]; then
        export KO_DONT_GRAB_INPUT="true"
    fi
fi

# check whether PLATFORM & PRODUCT have a value assigned by rcS
if [ -z "${PRODUCT}" ]; then
    # shellcheck disable=SC2046
    export $(grep -s -e '^PRODUCT=' "/proc/$(pidof -s udevd)/environ")
fi

if [ -z "${PRODUCT}" ]; then
    PRODUCT="$(/bin/kobo_config.sh 2>/dev/null)"
    export PRODUCT
fi

# PLATFORM is used in koreader for the path to the Wi-Fi drivers (as well as when restarting nickel)
if [ -z "${PLATFORM}" ]; then
    # shellcheck disable=SC2046
    export $(grep -s -e '^PLATFORM=' "/proc/$(pidof -s udevd)/environ")
fi

if [ -z "${PLATFORM}" ]; then
    PLATFORM="freescale"
    if dd if="/dev/mmcblk0" bs=512 skip=1024 count=1 | grep -q "HW CONFIG"; then
        CPU="$(ntx_hwconfig -s -p /dev/mmcblk0 CPU 2>/dev/null)"
        PLATFORM="${CPU}-ntx"
    fi

    if [ "${PLATFORM}" != "freescale" ] && [ ! -e "/etc/u-boot/${PLATFORM}/u-boot.mmc" ]; then
        PLATFORM="ntx508"
    fi
    export PLATFORM
fi

# Make sure we have a sane-ish INTERFACE env var set...
if [ -z "${INTERFACE}" ]; then
    # That's what we used to hardcode anyway
    INTERFACE="eth0"
    export INTERFACE
fi

# We'll enforce UR in ko_do_fbdepth, so make sure further FBInk usage (USBMS)
# will also enforce UR... (Only actually meaningful on sunxi).
if [ "${PLATFORM}" = "b300-ntx" ]; then
    export FBINK_FORCE_ROTA=0
    # On sunxi, non-REAGL waveform modes suffer from weird merging quirks...
    FBINK_WFM="REAGL"
    # And we also cannot use batched updates for the crash screen, as buffers are private,
    # so each invocation essentially draws in a different buffer...
    FBINK_BATCH_FLAG=""
    # Same idea for backgroundless...
    FBINK_BGLESS_FLAG="-B GRAY9"
    # It also means we need explicit background padding in the OT codepath...
    FBINK_OT_PADDING=",padding=BOTH"

    # Make sure we poke the right input device
    KOBO_TS_INPUT="/dev/input/by-path/platform-0-0010-event"
else
    FBINK_WFM="GL16"
    FBINK_BATCH_FLAG="-b"
    FBINK_BGLESS_FLAG="-O"
    FBINK_OT_PADDING=""
    KOBO_TS_INPUT="/dev/input/event1"
fi

# We'll want to ensure Portrait rotation to allow us to use faster blitting codepaths @ 8bpp,
# so remember the current one before fbdepth does its thing.
IFS= read -r ORIG_FB_ROTA <"/sys/class/graphics/fb0/rotate"
echo "Original fb rotation is set @ ${ORIG_FB_ROTA}" >>crash.log 2>&1

# In the same vein, swap to 8bpp,
# because 16bpp is the worst idea in the history of time, as RGB565 is generally a PITA without hardware blitting,
# and 32bpp usually gains us nothing except a performance hit (we're not Qt5 with its QPainter constraints).
# The reduced size & complexity should hopefully make things snappier,
# (and hopefully prevent the JIT from going crazy on high-density screens...).
# NOTE: Even though both pickel & Nickel appear to restore their preferred fb setup, we'll have to do it ourselves,
#       as they fail to flip the grayscale flag properly. Plus, we get to play nice with every launch method that way.
#       So, remember the current bitdepth, so we can restore it on exit.
IFS= read -r ORIG_FB_BPP <"/sys/class/graphics/fb0/bits_per_pixel"
echo "Original fb bitdepth is set @ ${ORIG_FB_BPP}bpp" >>crash.log 2>&1
# Sanity check...
case "${ORIG_FB_BPP}" in
    8) ;;
    16) ;;
    32) ;;
    *)
        # Uh oh? Don't do anything...
        unset ORIG_FB_BPP
        ;;
esac

# The actual swap is done in a function, because we can disable it in the Developer settings, and we want to honor it on restart.
ko_do_fbdepth() {
    ./fbdepth -d 8 -r -1 >>crash.log 2>&1
}

# Ensure we start with a valid nameserver in resolv.conf, otherwise we're stuck with broken name resolution (#6421, #6424).
# Fun fact: this wouldn't be necessary if Kobo were using a non-prehistoric glibc... (it was fixed in glibc 2.26).
ko_do_dns() {
    # If there aren't any servers listed, append CloudFlare's
    if ! grep -q '^nameserver' "/etc/resolv.conf"; then
        echo "nameserver 1.1.1.1" >>"/etc/resolv.conf"
    fi
}

# Remount the SD card RW if it's inserted and currently RO
if awk '$4~/(^|,)ro($|,)/' /proc/mounts | grep ' /mnt/sd '; then
    mount -o remount,rw /mnt/sd
fi

# we keep at most 500KB worth of crash log
if [ -e crash.log ]; then
    tail -c 500000 crash.log >crash.log.new
    mv -f crash.log.new crash.log
fi

ko_do_fbdepth
ko_do_dns


./typewriter "$@" >>crash.log 2>&1
RETURN_VALUE=$?

if [ -n "${ORIG_FB_BPP}" ]; then
    echo "Restoring original fb bitdepth @ ${ORIG_FB_BPP}bpp & rotation @ ${ORIG_FB_ROTA}" >>crash.log 2>&1
    ./fbdepth -d "${ORIG_FB_BPP}" -r "${ORIG_FB_ROTA}" >>crash.log 2>&1
else
    echo "Restoring original fb rotation @ ${ORIG_FB_ROTA}" >>crash.log 2>&1
    ./fbdepth -r "${ORIG_FB_ROTA}" >>crash.log 2>&1
fi

# Restore original CPUFreq governor if need be...
if [ -n "${ORIG_CPUFREQ_GOV}" ]; then
    echo "${ORIG_CPUFREQ_GOV}" >"${CPUFREQ_SYSFS_PATH}/scaling_governor"

    # NOTE: Leave DVFS alone, it'll be handled by Nickel if necessary.
fi

if [ "${VIA_NICKEL}" = "true" ]; then
    if [ "${FROM_KFMON}" = "true" ]; then
            ./nickel.sh &
        else
            ./nickel.sh &
        fi
    else
        # Otherwise, just restart Nickel
        ./nickel.sh &
    fi
else
    # if we were called from advboot then we must reboot to go to the menu
    # NOTE: This is actually achieved by checking if KSM or a KSM-related script is running:
    #       This might lead to false-positives if you use neither KSM nor advboot to launch KOReader *without nickel running*.
    if ! pkill -0 -f kbmenu; then
        /sbin/reboot
    fi
fi

# Wipe the clones on exit
rm -f "/tmp/typewriter.sh"

exit ${RETURN_VALUE}

