#!/bin/bash
source my-openrc.sh
export OS_PROJECT_DOMAIN_NAME=$OS_USER_DOMAIN_NAME
export OS_IDENTITY_API_VERSION="3"
export OS_CACERT=$(pwd)/OpenStack.cer
