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

BIN := $(APPDIR)/staging/libframework.a

CFLAGS += ${shell $(INCDIR) $(INCDIROPT) "$(CC)" $(APPDIR)/external/dbus/dbus}
CFLAGS += ${shell $(INCDIR) $(INCDIROPT) "$(CC)" $(APPDIR)/external/glib/glib}
CFLAGS += ${shell $(INCDIR) $(INCDIROPT) "$(CC)" $(APPDIR)/external/glib/glib/glib}
CFLAGS += ${shell $(INCDIR) $(INCDIROPT) "$(CC)" $(APPDIR)/frameworks/utils/gdbus}
CFLAGS += ${shell $(INCDIR) $(INCDIROPT) "$(CC)" $(APPDIR)/frameworks/telephony}

CSRCS += tapi_manager.c tapi_utils.c tapi_call.c tapi_data.c tapi_sim.c tapi_stk.c
CSRCS += tapi_cbs.c tapi_sms.c tapi_network.c tapi_ss.c tapi_ims.c

ifneq ($(CONFIG_TELEPHONY_TOOL),)
  MAINSRC   += telephony_tool.c
  PROGNAME  += telephonytool
  PRIORITY  += $(CONFIG_TELEPHONY_TOOL_PRIORITY)
  STACKSIZE += $(CONFIG_TELEPHONY_TOOL_STACKSIZE)
endif

include $(APPDIR)/Application.mk
