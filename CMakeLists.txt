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

if(CONFIG_TELEPHONY OR CONFIG_PHONE_SERVICE)
  set_property(
    TARGET nuttx
    APPEND
    PROPERTY NUTTX_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_LIST_DIR}/include)
  set(INCDIR)
  if(CONFIG_TELEPHONY)
    list(APPEND CSRCS
        ${CMAKE_CURRENT_LIST_DIR}/src/tapi_manager.c
        ${CMAKE_CURRENT_LIST_DIR}/src/tapi_call.c
        ${CMAKE_CURRENT_LIST_DIR}/src/tapi_data.c
        ${CMAKE_CURRENT_LIST_DIR}/src/tapi_sim.c
        ${CMAKE_CURRENT_LIST_DIR}/src/tapi_stk.c
        ${CMAKE_CURRENT_LIST_DIR}/src/tapi_cbs.c
        ${CMAKE_CURRENT_LIST_DIR}/src/tapi_sms.c
        ${CMAKE_CURRENT_LIST_DIR}/src/tapi_network.c
        ${CMAKE_CURRENT_LIST_DIR}/src/tapi_ss.c
        ${CMAKE_CURRENT_LIST_DIR}/src/tapi_ims.c
        ${CMAKE_CURRENT_LIST_DIR}/src/tapi_utils.c
        ${CMAKE_CURRENT_LIST_DIR}/src/tapi_phonebook.c)

    list(APPEND INCDIR
        ${NUTTX_APPS_DIR}/external/dbus/dbus
        ${NUTTX_APPS_DIR}/external/ofono/include
        ${NUTTX_APPS_DIR}/frameworks/system/utils/gdbus)
  endif()

  if(CONFIG_PHONE_SERVICE)
    list(APPEND CSRCS
        ${CMAKE_CURRENT_LIST_DIR}/src/tapi_phone.c)

    list(APPEND INCDIR
        ${NUTTX_APPS_DIR}/frameworks/connectivity/common/xpc/conn_xpc/include
        ${NUTTX_APPS_DIR}/frameworks/bluetooth/framework/include)
  endif()

  list(APPEND INCDIR ${NUTTX_APPS_DIR}/frameworks/connectivity/telephony)

  nuttx_add_library(tapi)

  if(CONFIG_TELEPHONY_TOOL)
    set(TOOL_SRCS ${CMAKE_CURRENT_LIST_DIR}/tools/telephony_tool.c)
    if(CONFIG_TELEPHONY)
      list(APPEND TOOL_SRCS ${CMAKE_CURRENT_LIST_DIR}/tools/telephony_esim_tool.c)
    endif()
    if(CONFIG_PHONE_SERVICE)
      list(APPEND TOOL_SRCS ${CMAKE_CURRENT_LIST_DIR}/tools/telephony_phone_tool.c)
    endif()
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
      ${TOOL_SRCS}
      INCLUDE_DIRECTORIES
      ${INCDIR}
      DEPENDS
      tapi)
  endif()

  if(CONFIG_TELEPHONY_TEST)
    file(GLOB APPEND_FILES ${CMAKE_CURRENT_LIST_DIR}/test/*.c)
    list(APPEND CSRCS ${APPEND_FILES})
    list(REMOVE_ITEM CSRCS ${CMAKE_CURRENT_LIST_DIR}/test/cmocka_telephony_test.c
                           ${CMAKE_CURRENT_LIST_DIR}/test/product_telephony_test.c)
    set(MAINSRC)

    if(CONFIG_GOLDFISH_RIL)
      list(APPEND MAINSRC test/cmocka_telephony_test.c)
    else()
      list(APPEND MAINSRC test/product_telephony_test.c)
    endif()

    nuttx_add_application(
      MODULE
      ${CONFIG_TELEPHONY_TEST}
      NAME
      cmocka_telephony_test
      STACKSIZE
      ${CONFIG_TELEPHONY_TEST_STACKSIZE}
      PRIORITY
      ${CONFIG_TELEPHONY_TEST_PRIORITY}
      SRCS
      ${MAINSRC}
      INCLUDE_DIRECTORIES
      ${INCDIR}
      DEPENDS
      tapi cmocka)
  endif()

  target_include_directories(tapi PRIVATE ${INCDIR})
  target_sources(tapi PRIVATE ${CSRCS})
endif()
