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

CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/external/dbus/dbus
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/external/ofono/include
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/system/utils/gdbus
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/connectivity/telephony

CSRCS += $(wildcard src/*.c)

ifneq ($(CONFIG_TELEPHONY_TOOL),)
  MAINSRC   += tools/telephony_tool.c
  PROGNAME  += telephonytool
  PRIORITY  += $(CONFIG_TELEPHONY_TOOL_PRIORITY)
  STACKSIZE += $(CONFIG_TELEPHONY_TOOL_STACKSIZE)
endif

ASRCS := $(wildcard $(ASRCS))
CSRCS := $(wildcard $(CSRCS))
CXXSRCS := $(wildcard $(CXXSRCS))
MAINSRC := $(wildcard $(MAINSRC))
NOEXPORTSRCS = $(ASRCS)$(CSRCS)$(CXXSRCS)$(MAINSRC)

ifneq ($(NOEXPORTSRCS),)
BIN := $(APPDIR)/staging/libframework.a
endif

ifneq ($(CONFIG_TELEPHONY_TEST),)
  CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/testing/cmocka/cmocka/include
  CSRCS  += $(filter-out test/*_telephony_test.c, $(wildcard test/*.c))

  PRIORITY  += $(CONFIG_TELEPHONY_TEST_PRIORITY)
  STACKSIZE += $(CONFIG_TELEPHONY_TEST_STACKSIZE)
  PROGNAME  += cmocka_telephony_test
  ifneq ($(CONFIG_GOLDFISH_RIL),)
  MAINSRC   += $(CURDIR)/test/cmocka_telephony_test.c
  else
  MAINSRC   += $(CURDIR)/test/product_telephony_test.c
  endif
endif

EXPORT_FILES := include

include $(APPDIR)/Application.mk
