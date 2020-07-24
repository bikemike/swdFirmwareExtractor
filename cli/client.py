#!/usr/bin/python3
#
# Copyright (C) 2017 Stefan Tatschner
#
# This Source Code Form is subject to the terms of the MIT License.
# If a copy of the MIT License was not distributed with this file,
# you can obtain one at https://opensource.org/licenses/MIT
#

import argparse
import os.path
from datetime import datetime
import subprocess
import signal
import time

from prompt_toolkit import prompt
from prompt_toolkit.completion import WordCompleter
from prompt_toolkit.history import InMemoryHistory
from prompt_toolkit.auto_suggest import AutoSuggestFromHistory


def auto_int(x):
    return int(x, 0)


def parse_args():
    parser = argparse.ArgumentParser(
        description='Firmware Extractor User Interface',
    )
    parser.add_argument(
        'SerialDeviceFILE',
        help='Device File to read from (e.g., /dev/ttyUSB0)'
    )
    parser.add_argument(
        '-i',
        '--interactive',
        action='store_true',
        help='Use interactive mode'
    )
    parser.add_argument(
        '-s',
        '--start',
        type=auto_int,
        default=0x00,
        help='Set start address',
    )
    parser.add_argument(
        '-l',
        '--length',
        type=auto_int,
        default=0x10000,
        help='Number of bytes to extract',
    )
    parser.add_argument(
        '-e',
        '--endianess',
        default='little',
        choices=['little', 'big'],
        help='Set endianess',
    )
    parser.add_argument(
        '-o',
        '--outfile',
        default='data-{}.bin'.format(datetime.now().strftime('%Y%m%d_%H%M')),
        help='Set output file path',
    )

    return parser.parse_args()


finish = False

def sigalarm_handler(signo, frame):
    # I want to print the statistics in the end of the program.
    # We can create an infinite loop with this sigalarm_handler,
    # as multiline mode of send_cmd() uses SIGALARM as well.
    # The global variable finish handles this case. And helps
    # us terminate the program when the timeout is fired a second
    # time.
    global finish
    if finish:
        print()
        print('Programm finished.')
        exit()

    print('End of data.')
    print()
    finish = True
    UART('/dev/ttyUSB0').send_cmd('P', True)


def decode_ascii(s, outfile):
    out = ''
    for i in range(0, len(s), 2):
        char = int(s[i:i+2], 16)
        outfile.write(char.to_bytes(1, 'little'))
        if char > 31 and char < 127:
            out += chr(char)
        else:
            out += '.'
    return out


def print_error(errcode):
    print()
    print('StatusCode: {:02X}'.format(errcode))

    if errcode == 0x20:
        print('Status OK')
    elif errcode == 0x40:
        print('Wait/Retry requested (bus access was not granted in time)')
    elif errcode == 0x06:
        print('Wait requested + additional OK (previous command OK, but no bus access)')
    elif errcode == 0x80:
        print('Fault during command execution (command error (access denied etc.))')
    elif errcode == 0xA0:
        print('Fault after successful command execution (reading invalid memory address?).')
    elif errcode == 0xE0:
        print('Failure during communication (check connection, no valid status reply received)')
    else:
        print('Unknown status code')


def _read_enddata(fd):
    buf = ''
    nchar = 0
    state = 'stat'
    print()
    # Print remaining data from uart until timeout is reached.
    while True:
        signal.alarm(1)
        char = fd.read(1)
        signal.alarm(0)

        print(char, end='')
        buf += char

        if char == '!':
            state = 'err'

        if state == 'err':
            if nchar == 25:
                print_error(int(buf[-8:], 16))
            nchar += 1


def read_ascii(fd, outfile):
    l = 0
    c = 0
    line = ''
    lineraw = ''
    outfile = open(outfile, 'ab', 16)

    while True:
        line += '0x{:08X}: '.format(16*l)
        decoded_line = ''

        while c < 32:
            char = fd.read(1)

            if char == ' ':
                continue

            lineraw += char

            if c % 2 == 0 and c != 0:
                line += ' '
            if c % 16 == 0 and c != 0:
                line += ' '

            # Reached end of data.
            # Flush all buffers and read special stuff at the end.
            if char == '\r' or char == '\n':
                try:
                    line += '  |' + decode_ascii(lineraw, outfile) + '|'
                except ValueError:
                    pass
                print(line)
                outfile.flush()
                outfile.close()
                _read_enddata(fd)
            c += 1
            line += char

        line += '  |' + decode_ascii(lineraw, outfile) + '|'

        print(line)
        line = ''
        lineraw = ''
        l += 1
        c = 0


class UART:

    def __init__(self, devnode):
        self.devnode = devnode
        subprocess.call( [
            'stty',
            '--file={}'.format(self.devnode),
            '115200',
            'raw',
            '-echok',
            '-echo',
        ])

    def send_cmd(self, code, multiline=False):
        readfd = open(self.devnode, 'r')
        writefd = open(self.devnode, 'w')
        time.sleep(0.1)
        writefd.write(code + '\n')

        while True:
            try:
                if multiline:
                    signal.alarm(1)
                char = readfd.read(1)
                signal.alarm(0)
                if not multiline:
                    if char == '\r' or char == '\n':
                        print()
                        break
                print(char, end='')
            except UnicodeDecodeError:
                print('Received garbage from target.')
                print('Is it already running?')
                print('Shutting down...')
                exit(1)

        writefd.close()
        readfd.close()

    def open(self, mode):
        return open(self.devnode, mode)


class REPL:

    def __init__(self, start=0x00, length=0x10000, mode='bin',
                 byteorder='little', devnode='/dev/ttyUSB0'):
        self.history = InMemoryHistory()
        self.promt = '> '
        self.config = {
            'start': start,
            'length': length,
            'byteorder': byteorder,
            'outfile': 'data-{}.bin'.format(datetime.now().strftime('%Y%m%d_%H%M')),
        }
        self.uart = UART(devnode)

    def handle_cmd(self, cmd, *args):
        if cmd == 'set':
            self.set_config(args[0], args[1])
        elif cmd == 'run':
            self.apply_config()
            self.uart.send_cmd('S')
            fd = self.uart.open('r')
            print()
            read_ascii(fd, self.config['outfile'])
            fd.close()
        elif cmd == 'cmd':
            self.uart.send_cmd(args[0], True)
        elif cmd == 'help':
            self.show_help()
        elif cmd == 'exit':
            print('Exit command received. Leaving...')
            exit(0)
        else:
            print('Error: Unknown command.')
            print("Try 'help'")

    def show_config(self):
        print('# Current Configuration')
        lines = []
        if self.config:
            tpl = '  {:<12}: '
            for key, val in self.config.items():
                if isinstance(val, int):
                    fstr = tpl + '0x{:X}'
                else:
                    fstr = tpl + '{}'
                lines.append(fstr.format(key, val))
            print('\n'.join(sorted(lines)))

    def show_help(self):
        print('# Supported commands')
        print('  set KEY VAL : Set configuration value KEY to VAL')
        print('  cmd CODE    : Send command code to UART')
        print('  run         : Start reading out code')
        print('  help        : Show this help page')
        print('  exit        : Terminate programm')

    def set_config(self, key, val):
        if key == 'byteorder':
            if val not in ('little', 'big'):
                print('Error: Wrong byteorder. Choose "little" or "big".')
                return
        elif key == 'start':
            val = int(val, 0)
            if val > 0x10000:
                print('Error: Start address is too large.')
                return
        elif key == 'length':
            val = int(val, 0)
        elif key == 'outfile':
            self.config[key] = str(val)
        else:
            print('Error: Config key does not exist.')
            return

        self.config[key] = val
        self.show_config()

    def apply_config(self):
        self.uart.send_cmd('A{:X}'.format(self.config['start']))
        self.uart.send_cmd('L{:X}'.format(self.config['length']))
        # Hardcode HEX mode
        self.uart.send_cmd('H')
        cmd = 'e' if self.config['byteorder'] == 'little' else 'E'
        self.uart.send_cmd(cmd)

    def run_loop(self):
        self.show_help()
        print()
        self.show_config()

        try:
            while True:
                cmd = prompt(self.promt,
                    history=self.history,
                    auto_suggest=AutoSuggestFromHistory(),
                )

                cmd = cmd.strip().split()
                cmd = cmd if cmd else ''

                if cmd != '':
                    self.handle_cmd(*cmd)
        except KeyboardInterrupt:
            print('KeyboardInterrupt received. Shutting down...')
            exit(1)
        except EOFError:
            print('Reached end of line. Leaving...')
            exit(0)


def main():
    args = parse_args()
    signal.signal(signal.SIGALRM, sigalarm_handler)

    if not os.path.exists(args.SerialDeviceFILE):
        print('Error: No such file.')
        exit(1)

    if args.interactive:
        REPL(devnode=args.SerialDeviceFILE).run_loop()
        exit(0)

    # If the script is not in interactive mode, issue this stuff
    # manually.
    uart = UART(args.SerialDeviceFILE)
    uart.send_cmd('A{:X}'.format(args.start))
    uart.send_cmd('L{:X}'.format(args.length))
    uart.send_cmd('H')  # Hardcode HEX mode
    if args.endianess == 'big':
        uart.send_cmd('E')
    else:
        uart.send_cmd('e')
    uart.send_cmd('S')
    fd = uart.open('r')
    print()

    try:
        read_ascii(fd, args.outfile)
    except KeyboardInterrupt:
        print('Leaving...')
    finally:
        fd.close()


if __name__ == '__main__':
    main()
