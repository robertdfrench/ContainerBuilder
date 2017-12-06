#!/bin/bash -e

echo "## Testing building with spack"
./tests/integration/build_with_spack.sh

echo "## Testing appropriate clobbering"
./tests/integration/test_bring_up_queue_doesnt_clobber_unrelated_boxes.sh
