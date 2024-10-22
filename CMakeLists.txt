#
# Copyright (C) 2021 Xiaomi Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License. You may obtain a copy of
# the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.
#

if(CONFIG_TELEPHONY)
  set_property(
    TARGET nuttx
    APPEND
    PROPERTY NUTTX_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_LIST_DIR}/include)
  set(CSRCS)
  file(GLOB APPEND_FILES ${CMAKE_CURRENT_LIST_DIR}/*.c)
  list(APPEND CSRCS ${APPEND_FILES})
  list(REMOVE_ITEM CSRCS ${CMAKE_CURRENT_LIST_DIR}/telephony_tool.c)
  set(INCDIR
      ${NUTTX_APPS_DIR}/external/dbus/dbus
      ${NUTTX_APPS_DIR}/external/ofono/include
      ${NUTTX_APPS_DIR}/frameworks/system/utils/gdbus
      ${NUTTX_APPS_DIR}/frameworks/telephony)

  nuttx_add_library(tapi)

  if(CONFIG_TELEPHONY_TOOL)
    nuttx_add_application(
      MODULE
      ${CONFIG_TELEPHONY_TOOL}
      NAME
      telephonytool
      STACKSIZE
      ${CONFIG_TELEPHONY_TOOL_STACKSIZE}
      PRIORITY
      ${CONFIG_TELEPHONY_TOOL_PRIORITY}
      SRCS
      telephony_tool.c
      INCLUDE_DIRECTORIES
      ${INCDIR}
      DEPENDS
      tapi)
  endif()

  target_include_directories(tapi PRIVATE ${INCDIR})
  target_sources(tapi PRIVATE ${CSRCS})
endif()
