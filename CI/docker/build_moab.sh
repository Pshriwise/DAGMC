#!/bin/bash

set -ex

source ${docker_env}

if [ ${MOAB_VERSION} == "master" ] || [ ${MOAB_VERSION} == "develop" ]; then
  branch=${MOAB_VERSION}
else
#  branch=Version${MOAB_VERSION}
  branch=${MOAB_VERSION}
fi

rm -rf ${moab_build_dir}/bld ${moab_install_dir}
mkdir -p ${moab_build_dir}/bld
cd ${moab_build_dir}
#git clone --depth 1 https://bitbucket.org/fathomteam/moab -b ${branch}
git clone https://bitbucket.org/fathomteam/moab 
cd moab
git checkout ${branch}
autoreconf -fi
cd ../bld
../moab/configure --enable-pymoab \
                 --enable-shared \
                 --enable-optimize \
                 --disable-debug \
                 --disable-fortran \
                 --disable-blaslapack \
                 --with-hdf5=${hdf5_install_dir} \
                 --prefix=${moab_install_dir} \
                 CC=${CC} CXX=${CXX}
make -j${ci_jobs}
make install
cd
rm -rf ${moab_build_dir}
