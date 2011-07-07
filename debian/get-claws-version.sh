#!/bin/bash
egrep '#define VERSION[[:space:]]+"' /usr/include/claws-mail/common/version.h|cut -d '"' -f 2
