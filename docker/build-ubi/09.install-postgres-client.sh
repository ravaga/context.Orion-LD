#!/bin/bash

# Copyright 2014 Telefonica Investigacion y Desarrollo, S.A.U
#
# This file is part of Orion Context Broker.
#
# Orion Context Broker is free software: you can redistribute it and/or
# modify it under the terms of the GNU Affero General Public License as
# published by the Free Software Foundation, either version 3 of the
# License, or (at your option) any later version.
#
# Orion Context Broker is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
# General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with Orion Context Broker. If not, see http://www.gnu.org/licenses/.
#
# For those usages not covered by this license please contact with
# iot_support at tid dot es
set -e

echo "---------- Yum utils ----------"
yum -y --nogpgcheck install https://download.postgresql.org/pub/repos/yum/reporpms/EL-8-x86_64/pgdg-redhat-repo-latest.noarch.rpm

yum -y install yum-utils perl-IO-Tty perl-IPC-Run perl-Test-Simple 

echo "---------- Add repo rpms ----------"
yum -y install blas libdap SuperLU lapack armadillo hdf5 xerces-c gdal-libs 

dnf -y module disable postgresql

yum -y --nogpgcheck install postgresql12 postgresql12-contrib

echo "---------- Install  postgres ----------"
yum -y install libpqxx-devel postgresql12-devel postgresql12-libs

echo "---------- install-postgres-client.sh is DONE ----------"