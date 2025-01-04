#!/bin/sh
# SPDX-License-Identifier: LGPL-3.0-or-later
#
# Copyright 2023, DataDirect Networks, Inc. All rights reserved.
# Author: Martin Schwenke <mschwenke@ddn.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program.  If not, see <http://www.gnu.org/licenses/>.
#

# Install ganesha build dependencies for the given distributions.
# This does not checked supported versions - that is done by
# ganesha-container.  This will obviously need continuous updates to
# support new distro versions.
#
# Please maintain this as a plain /bin/sh script that passes
# ShellCheck.
#
# The containers could be much lighter weight without the Python Qt
# dependency.
#
# Formatted with: shfmt -w -p -i 0 -fn <file>
#

set -e

install_debian()
{
	export DEBIAN_FRONTEND="noninteractive"

	apt-get update

	libnsl_pkg="libnsl-dev"
	case "$ID" in
	debian)
		case "$VERSION_ID" in
		10)
			libnsl_pkg=""
			;;
		esac
		;;
	ubuntu)
		case "$VERSION_ID" in
		18.04 | 20.04)
			libnsl_pkg=""
			;;
		esac
		;;
	esac

	# shellcheck disable=SC2086
	# $libnsl_pkg may be empty
	apt-get install -y \
		bison \
		build-essential \
		cmake \
		debianutils \
		doxygen \
		flex \
		g++ \
		gcc \
		git \
		libacl1-dev \
		libblkid-dev \
		libcap-dev \
		libdbus-1-dev \
		libjemalloc-dev \
		libkrb5-dev \
		$libnsl_pkg \
		liburcu-dev \
		python3 \
		pyqt5-dev-tools \
		rsync \
		sudo \
		uuid-dev
}

install_rh()
{
	case "$ID" in
	fedora)
		:
		;;
	*)
		yum install -y epel-release
		;;
	esac

	python_pkg="python3"
	extra_repos="powertools"
	case "$ID" in
	almalinux)
		case "$VERSION_ID" in
		9.*)
			extra_repos="crb"
			;;
		esac
		;;
	centos)
		case "$VERSION_ID" in
		7)
			python_pkg="python36"
			extra_repos=""
			;;
		9)
			extra_repos="crb"
			;;
		esac
		;;
	fedora)
		case "$VERSION_ID" in
		*)
			extra_repos=""
			;;
		esac
		;;
	rocky)
		case "$VERSION_ID" in
		9.*)
			extra_repos="devel"
			;;
		esac
		;;
	esac

	if [ -n "$extra_repos" ]; then
		yum install -y 'dnf-command(config-manager)'

		# shellcheck disable=SC2086
		# $extra_repos can be multi-word
		yum config-manager --set-enabled -y $extra_repos
	fi

	yum install -y \
		"@Development Tools" \
		bison \
		cmake \
		dbus-devel \
		doxygen \
		flex \
		gcc-c++ \
		jemalloc-devel \
		krb5-devel \
		libacl-devel \
		libasan \
		libblkid-devel \
		libcap-devel \
		libnfsidmap-devel \
		libnsl2-devel \
		libuuid-devel \
		"$python_pkg" \
		"${python_pkg}-qt5-devel" \
		rsync \
		sudo \
		userspace-rcu-devel \
		which
}

#
# Get release family
#

. /etc/os-release

case "$ID" in
almalinux | centos | rhel | rocky | fedora)
	family="rh"
	;;
debian | ubuntu)
	family="debian"
	;;
*)
	echo "Unsupported distro: ${ID}"
	exit 1
	;;
esac

#
# Run installation function for family
#

"install_${family}"
