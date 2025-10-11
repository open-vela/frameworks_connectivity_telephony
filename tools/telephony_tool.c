/****************************************************************************
 * framework/telephony/telephony_tool.c
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/
#include "tapi_tool.h"
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <system/readline.h>

static async_message_t g_uv_message;
static bool g_should_exit;

int split_input(char dst[][MAX_INPUT_ARGS_LEN], int size, char* str, const char* spl)
{
    char* result;
    char* p_save;
    int n = 0;

    result = strtok_r(str, spl, &p_save);
    while (result != NULL && strlen(result) < MAX_INPUT_ARGS_LEN) {
        if (n < size)
            strcpy(dst[n], result);

        n++;
        result = strtok_r(NULL, spl, &p_save);
    }

    return n;
}

static void exit_handler(int signo)
{
    g_should_exit = true;
    printf("telephonytool exit successful\n");
}

static void telephonytool_menu(void)
{
    printf("=============  Telephony Tool Manual =============\n");
    printf("***** 1: Radio TAPI Instruction              *****\n");
    printf("***** 2: Call TAPI Instruction               *****\n");
    printf("***** 3: Data TAPI Instruction               *****\n");
    printf("***** 4: SIM TAPI Instruction                *****\n");
    printf("***** 5: SMS & CBS TAPI Instruction          *****\n");
    printf("***** 6: Network TAPI Instruction            *****\n");
    printf("***** 7: SS TAPI Instruction                 *****\n");
    printf("***** 8: IMS TAPI Instruction                *****\n");
    printf("***** 9: Phonebook TAPI Instruction          *****\n");
    printf("***** 10: STK TAPI Instruction               *****\n");
    printf("***** 11: TAPI open&close Instruction        *****\n");
    printf("***** 12: PHONE SERVICE TAPI Instruction     *****\n");
    printf("***** 13: Quit                               *****\n");
    printf("***** 14: Help                               *****\n");
    printf("Please enter your choice: (1~14) \n");
}

static struct commontool_cmd_s common_cmds[] = {
    { "q", QUIT_CMD, NULL, "Quit (pls enter : q)" },
    { "help", HELP_CMD, NULL,
        "Show this message (pls enter : help)" },
    { 0 },
};

static void show_common_cmd(int num)
{
    int i;

    for (i = 0; common_cmds[i].cmd; i++) {
        if (common_cmds[i].type == num) {
            printf("%-35s %s\n", common_cmds[i].cmd,
                common_cmds[i].help);
        }
    }
    printf("\n");
}

static void telephonytool_handle_choice(int num)
{
    if (num == PHONE_SERVICE_CMD) {
#ifdef CONFIG_PHONE_SERVICE
        show_tapi_phoneservice_cmd();
#else
        printf("no support phoneservice cmd,%s\n", __func__);
#endif
    } else if (num == HELP_CMD || num == QUIT_CMD) {
        show_common_cmd(num);
    } else {
#ifdef CONFIG_TELEPHONY
        show_tapi_telephony_cmd(num);
#else
        printf("no support tele cmd,%s\n", __func__);
#endif
    }
}

static int telephonytool_cmd_help(void)
{
    int num;

    telephonytool_menu();
    scanf("%d", &num);
    printf("\n");
    if (num < RADIO_CMD || num > HELP_CMD) {
        printf("Invalid input!\n");
    } else {
        telephonytool_handle_choice(num);
    }
    return 0;
}

static void execute_telephonytool_exit(void)
{
    g_should_exit = true;
    uv_close((uv_handle_t*)&g_uv_message.async, NULL);
#ifdef CONFIG_TELEPHONY
    if (!g_context_is_null()) {
        tapi_clean();
        update_uv_exit_flag();
    } else {
        syslog(LOG_ERR, "tapi is already close, stop default loop");
        uv_stop(uv_default_loop());
    }
#endif
#ifdef CONFIG_PHONE_SERVICE
    phone_client_clean();
#endif
}

static void uv_async_callback(uv_async_t* handle)
{
    bool find_flag = false;
    async_message_t* msg = (async_message_t*)handle->data;

    printf("telephonytool %s,%s\n", __func__, msg->cmd);
    if (strcmp(msg->cmd, "exit") == 0) {
        execute_telephonytool_exit();
    } else {
#ifdef CONFIG_PHONE_SERVICE
        find_flag = execute_phone_service_cmd(msg->cmd, msg->param);
        if (find_flag) {
            return;
        }
#endif
#ifdef CONFIG_TELEPHONY
        find_flag = execute_telephony_cmd(msg->cmd, msg->param);
        if (find_flag) {
            return;
        }
#endif
        printf("%s,cmd not support\n", __func__);
    }
}

static void* read_stdin(pthread_addr_t pvarg)
{
    int arg_len, len;
    char *cmd, *arg, *buffer;

    buffer = malloc(CONFIG_NSH_LINELEN);
    if (buffer == NULL) {
        return NULL;
    }

    while (!g_should_exit) {
        printf("telephonytool> ");
        fflush(stdout);

        len = readline_stream(buffer, CONFIG_NSH_LINELEN, stdin, stdout);
        buffer[len] = '\0';
        if (len < 0)
            continue;

        if (buffer[0] == '!') {
#ifdef CONFIG_SYSTEM_SYSTEM
            system(buffer + 1);
#endif
            continue;
        }

        if (buffer[len - 1] == '\n')
            buffer[len - 1] = '\0';
        cmd = strtok_r(buffer, " \n", &arg);

        if (cmd == NULL)
            continue;

        if (strncmp(cmd, "help", 4) == 0) {
            telephonytool_cmd_help();
            continue;
        }

        while (*arg == ' ')
            arg++;

        arg_len = strlen(arg);
        while (isspace(arg[arg_len - 1]))
            arg_len--;

        if (strcmp(cmd, "q") == 0)
            break;

        arg[arg_len] = '\0';

        memset(g_uv_message.cmd, '\0', sizeof(g_uv_message.cmd));
        snprintf(g_uv_message.cmd, sizeof(g_uv_message.cmd), "%s", cmd);
        memset(g_uv_message.param, '\0', sizeof(g_uv_message.param));
        snprintf(g_uv_message.param, sizeof(g_uv_message.param), "%s", arg);
        g_uv_message.async.data = (void*)&g_uv_message;
        uv_async_send(&g_uv_message.async);
    }

    free(buffer);
    memset(g_uv_message.cmd, '\0', sizeof(g_uv_message.cmd));
    strcpy(g_uv_message.cmd, "exit");
    g_uv_message.async.data = (void*)&g_uv_message;
    uv_async_send(&g_uv_message.async);
    return NULL;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, char* argv[])
{
    struct sched_param param;
    pthread_attr_t attr;
    pthread_t thread;
    int ret;

    g_should_exit = false;
    if (signal(SIGINT, exit_handler) == SIG_ERR) {
        return -errno;
    }

#ifdef CONFIG_PHONE_SERVICE
    ret = phone_client_init();
    if (ret < 0) {
        printf("error:phone service client init fail\n");
    }
#endif

    uv_async_init(uv_default_loop(), &g_uv_message.async, uv_async_callback);

    pthread_attr_init(&attr);
    param.sched_priority = CONFIG_TELEPHONY_TOOL_PRIORITY;
    pthread_attr_setschedparam(&attr, &param);
    pthread_attr_setstacksize(&attr, CONFIG_TELEPHONY_TOOL_STACKSIZE);

#ifdef CONFIG_TELEPHONY
    char* dbus_name = TAPI_DBUS_NAME_DEFAULT;

    if (argc == 2)
        dbus_name = argv[1];
    ret = tapi_init(dbus_name);
    if (ret < 0) {
        printf("telephonytool: failed to open tapi context\n");
#ifdef CONFIG_PHONE_SERVICE
        phone_client_clean();
#endif
        return 0;
    }
#endif
    ret = pthread_create(&thread, &attr, read_stdin, NULL);
    if (ret != 0) {
#ifdef CONFIG_TELEPHONY
        tapi_clean();
#endif
#ifdef CONFIG_PHONE_SERVICE
        phone_client_clean();
#endif
        printf("telephonytool: failed to create thread\n");
        return ret;
    }

    uv_run(uv_default_loop(), UV_RUN_DEFAULT);
    uv_loop_close(uv_default_loop());

    pthread_join(thread, NULL);
#ifdef CONFIG_TELEPHONY
    tapi_clean();
#endif
#ifdef CONFIG_PHONE_SERVICE
    phone_client_clean();
#endif

    return ret;
}
