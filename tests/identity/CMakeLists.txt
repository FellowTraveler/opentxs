# Copyright (c) 2010-2021 The Open-Transactions developers
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

add_opentx_test(unittests-opentxs-identity Test_Nym.cpp)
target_compile_definitions(
  unittests-opentxs-identity
  PRIVATE
    "OT_STORAGE_LMDB=${LMDB_EXPORT}"
    "OT_STORAGE_FS=${FS_EXPORT}"
    "OT_STORAGE_SQLITE=${SQLITE_EXPORT}"
)
