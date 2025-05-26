############################################################################
# frameworks/telephony/Makefile
#
# Licensed to the Apache Software Foundation (ASF) under one or more
# contributor license agreements.  See the NOTICE file distributed with
# this work for additional information regarding copyright ownership.  The
# ASF licenses this file to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance with the
# License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
# License for the specific language governing permissions and limitations
# under the License.
#
############################################################################

include $(APPDIR)/Make.defs

ifneq ($(CONFIG_TELEPHONY),)
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/external/dbus/dbus
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/external/ofono/include
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/system/utils/gdbus
endif

ifneq ($(CONFIG_PHONE_SERVICE),)
CFLAGS += -I$(APPDIR)/frameworks/connectivity/common/xpc/conn_xpc/include
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/bluetooth/framework/include
endif

CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/connectivity/telephony

ifneq ($(CONFIG_TELEPHONY),)
CSRCS += src/tapi_manager.c src/tapi_call.c src/tapi_data.c src/tapi_sim.c src/tapi_stk.c src/tapi_utils.c
CSRCS += src/tapi_cbs.c src/tapi_sms.c src/tapi_network.c src/tapi_ss.c src/tapi_ims.c src/tapi_phonebook.c
endif

ifneq ($(CONFIG_PHONE_SERVICE),)
CSRCS += src/tapi_phone.c
endif

ifneq ($(CONFIG_TELEPHONY_TOOL),)
  MAINSRC   += tools/telephony_tool.c
  ifneq ($(CONFIG_TELEPHONY),)
    CSRCS += tools/telephony_esim_tool.c
  endif
  ifneq ($(CONFIG_PHONE_SERVICE),)
    CSRCS += tools/telephony_phone_tool.c
  endif
  PROGNAME  += telephonytool
  PRIORITY  += $(CONFIG_TELEPHONY_TOOL_PRIORITY)
  STACKSIZE += $(CONFIG_TELEPHONY_TOOL_STACKSIZE)
endif

ifneq ($(CONFIG_TELEPHONY_TEST),)
  CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/testing/cmocka/cmocka/include
  CSRCS  += $(filter-out test/cmocka_telephony_test.c test/product_telephony_test.c \
              test/remote_operation.c, $(wildcard test/*.c))

  ifneq ($(CONFIG_GOLDFISH_RIL),)
  CSRCS     += test/remote_operation.c
  MAINSRC   += $(CURDIR)/test/cmocka_telephony_test.c
  else
  MAINSRC   += $(CURDIR)/test/product_telephony_test.c
  endif
  PROGNAME  += cmocka_telephony_test
  PRIORITY  += $(CONFIG_TELEPHONY_TEST_PRIORITY)
  STACKSIZE += $(CONFIG_TELEPHONY_TEST_STACKSIZE)

depend::
	$(Q) touch $(CSRCS)
endif

EXPORT_FILES := include

include $(APPDIR)/Application.mk
