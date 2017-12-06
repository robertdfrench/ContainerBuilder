#!/bin/bash -ex
ARTIFACTS=tests/artifacts/build_with_spack
mkdir -p $ARTIFACTS
export HOME=$ARTIFACTS
rm -rf $HOME/.spack

if [ ! -d $HOME/spack ]; then
	git clone https://github.com/spack/spack.git $HOME/spack
fi

$HOME/spack/bin/spack repo add spack-repo/containerbuilder
$HOME/spack/bin/spack install --no-checksum container-builder
