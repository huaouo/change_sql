//
// Created by huaouo on 2021/12/19.
//

#include <stdint.h>
#include <stdio.h>
#include "flags.h"
#include <fmt/format.h>

int main() {
    uint32_t flag = 0x01FFA605;

    uint32_t flag2 = CLIENT_LONG_PASSWORD | CLIENT_LONG_FLAG | CLIENT_PROTOCOL_41
                     | CLIENT_INTERACTIVE | CLIENT_TRANSACTIONS
                     | CLIENT_RESERVED2 | CLIENT_MULTI_STATEMENTS | CLIENT_MULTI_RESULTS | CLIENT_PS_MULTI_RESULTS |
                     CLIENT_PLUGIN_AUTH
                     | CLIENT_CONNECT_ATTRS | CLIENT_PLUGIN_AUTH_LENENC_CLIENT_DATA |
                     CLIENT_CAN_HANDLE_EXPIRED_PASSWORDS
                     | CLIENT_SESSION_TRACK | CLIENT_DEPRECATE_EOF;

    int x = 0x86FFF7DF & CLIENT_RESERVED2 != 0;
    uint32_t flag3 = flag2 & ~CLIENT_PLUGIN_AUTH_LENENC_CLIENT_DATA;
    printf("%x\n", flag3);

    fmt::print("{:x}", 23);
}
