#!/bin/sh
set -eu
project_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$project_dir"
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build -j 4
ctest --test-dir build --output-on-failure
version=$(tr -d '\r\n' < VERSION | sed 's/-/~/')
if [ -n "${DISCFORGE_UBUNTU_VERSION:-}" ]; then
    case "$DISCFORGE_UBUNTU_VERSION" in
        22.04|24.04|26.04) version="${version}+ubuntu${DISCFORGE_UBUNTU_VERSION}" ;;
        *) echo "Unsupported Ubuntu target" >&2; exit 1 ;;
    esac
fi
arch=$(dpkg --print-architecture)
package_root="$project_dir/build/deb-root"
cmake -E rm -rf "$package_root"
mkdir -p "$package_root/DEBIAN" "$project_dir/dist"
DESTDIR="$package_root" cmake --install build --prefix /usr
strip "$package_root/usr/bin/discforge-native"
depends=$(dpkg-shlibdeps -O -e "$package_root/usr/bin/discforge-native" | sed -n 's/^shlibs:Depends=//p')
installed_size=$(du -sk "$package_root/usr" | cut -f1)
cat > "$package_root/DEBIAN/control" <<CONTROL
Package: discforge
Version: $version
Section: utils
Priority: optional
Architecture: $arch
Maintainer: DiscForge contributors <noreply@localhost>
Installed-Size: $installed_size
Depends: $depends, qt6-qpa-plugins, wodim, genisoimage, dvd+rw-tools, udisks2, util-linux, eject
Description: Lightweight native optical disc burner
 Multisession CD/DVD writing with read-back checksum verification.
CONTROL
find "$package_root" -type d -exec chmod 0755 {} +
(
 cd "$package_root"
 find usr -type f -print0 | sort -z | xargs -0 md5sum > DEBIAN/md5sums
)
# GitHub Release asset names do not preserve '~'. Keep Debian's internal
# version ordering, but use an ASCII release-safe archive filename.
asset_version=$(printf '%s' "$version" | tr '~' '-')
dpkg-deb --root-owner-group -Zxz --build "$package_root" "$project_dir/dist/discforge_${asset_version}_${arch}.deb"
