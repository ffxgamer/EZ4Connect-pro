#!/bin/bash
# Invoked only after the download has passed its SHA-256 check.
set -eu
work=$1
target=$2
parent_pid=$3
version=$4
resume_token=${5:-}
mountpoint="$work/mount"
parent_dir=$(/usr/bin/dirname "$target")
candidate="$parent_dir/.EZ4Connect-update-$parent_pid.app"
backup="$parent_dir/.EZ4Connect-backup-$parent_pid.app"
mounted=0
moved_old=0
installed=0

fail() {
    status=$?
    trap - EXIT
    if [ "$mounted" = 1 ]; then /usr/bin/hdiutil detach "$mountpoint" -quiet || true; fi
    if [ "$status" -ne 0 ]; then
        if [ "$moved_old" = 1 ]; then
            if [ "$installed" = 1 ]; then /bin/rm -rf "$target"; fi
            /bin/mv "$backup" "$target" || true
        fi
        /bin/rm -rf "$candidate"
        if [ -n "$resume_token" ]; then
            /usr/bin/open -n "$target" --args --update-error "更新未完成，已尝试恢复原版本。" "--resume-vpn=$resume_token" || true
        else
            /usr/bin/open -n "$target" --args --update-error "更新未完成，已尝试恢复原版本。" || true
        fi
    fi
    /bin/rm -rf "$work"
    exit "$status"
}
trap fail EXIT

for attempt in $(/usr/bin/jot 60); do
    if ! /bin/kill -0 "$parent_pid" 2>/dev/null; then break; fi
    /bin/sleep 1
done
if /bin/kill -0 "$parent_pid" 2>/dev/null; then exit 1; fi
test ! -e "$candidate"
test ! -e "$backup"
/bin/mkdir "$mountpoint"
/usr/bin/hdiutil attach "$work/update.dmg" -readonly -nobrowse -quiet -mountpoint "$mountpoint"
mounted=1
source_app="$mountpoint/EZ4Connect Pro.app"
test -d "$source_app/Contents/MacOS"
bundle_id=$(/usr/libexec/PlistBuddy -c 'Print :CFBundleIdentifier' "$source_app/Contents/Info.plist")
bundle_version=$(/usr/libexec/PlistBuddy -c 'Print :CFBundleShortVersionString' "$source_app/Contents/Info.plist")
test "$bundle_id" = 'com.github.chenx-dust.EZ4Connect'
test "$bundle_version" = "$version"
/usr/bin/codesign --verify --deep --strict "$source_app"
/usr/bin/ditto "$source_app" "$candidate"
/usr/bin/xattr -dr com.apple.quarantine "$candidate"
/usr/bin/codesign --verify --deep --strict "$candidate"
/usr/bin/hdiutil detach "$mountpoint" -quiet
mounted=0
/bin/mv "$target" "$backup"
moved_old=1
/bin/mv "$candidate" "$target"
installed=1
if [ -n "$resume_token" ]; then
    /usr/bin/open -n "$target" --args --update-complete "$version" "--resume-vpn=$resume_token"
else
    /usr/bin/open -n "$target" --args --update-complete "$version"
fi
/bin/rm -rf "$backup"
moved_old=0
