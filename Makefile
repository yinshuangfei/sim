BUILD_DIR=build


all: run

.PHONY: pre
pre:
	[ -e ${BUILD_DIR} ] || mkdir ${BUILD_DIR}
	cd ${BUILD_DIR} && cmake \
		-DUSE_FSAL_PROXY_V4=OFF \
		-DUSE_FSAL_PROXY_V3=OFF \
		-DUSE_FSAL_LUSTRE=OFF \
		-DUSE_FSAL_LIZARDFS=OFF \
		-DUSE_FSAL_KVSFS=OFF \
		-DUSE_FSAL_CEPH=OFF \
		-DUSE_FSAL_GPFS=OFF \
		-DUSE_FSAL_XFS=OFF \
		-DUSE_FSAL_GLUSTER=OFF \
		-DUSE_FSAL_NULL=OFF \
		-DUSE_FSAL_RGW=OFF \
		-DUSE_FSAL_MEM=OFF \
		-DUSE_FSAL_VFS=OFF \
		-DCMAKE_BUILD_TYPE=Debug \
		-DENABLE_VFS_POSIX_ACL=OFF \
		-DRPCBIND=ON \
		-DUSE_SYSTEM_NTIRPC=ON \
		../nfs-ganesha-5.7/src/

build: pre
	cd ${BUILD_DIR} && make -j`nproc`

install: build
	cp build/ganesha.nfsd /usr/bin/ganesha.nfsd
	cp ./build/MainNFSD/libganesha_nfsd.so.5.7 /usr/lib64/libganesha_nfsd.so.5.7
	cp ./build/MainNFSD/libganesha_nfsd.so /usr/lib64/libganesha_nfsd.so
	cp build/FSAL/FSAL_SIM/libfsalsim.so /usr/lib64/ganesha/

run: install
	mkdir -p /run/ganesha; ganesha.nfsd -F -L /var/log/ganesha/ganesha.log -f config/sim.conf -N NIV_CRIT

.PHONY: kill
kill:
	ps -ef | grep ganesha.nfsd | grep -v grep | awk '{print $$2}' | xargs -i kill -9 {}

.PHONY: clean
clean:
	rm -fr ${BUILD_DIR}
