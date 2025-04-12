#!/usr/bin/env python3
import re
import sys
import itertools

matcher = r'rx,     0x([0-9A-F]{3}),([1-8]);([^,]*),([0-9.]+),([0-9]+;00;00)'

def hex_byte_to_bin(data):
    return bin(int(data, 16))[2:].zfill(8)

def interpret(function_code, cob_id, length, data):
    if function_code == '080':
        if cob_id == '00':
            return 'sync'
        else:
            return 'heartbeat? ' + str(data)

    if function_code == '100':
        return 'timestamp ' + str(data)

    if function_code == '580':
        res = 'SDO transmit '
        b0 = hex_byte_to_bin(data[0])
        ccs = b0[:3]
        n = b0[4:6]
        e = b0[6:7]
        s = b0[7:8]
        res += 'ccs=%s, n=%s e=%s, s=%s, ' % (ccs, n, e, s)
        res += 'index=%s %s, ' % (data[1], data[2])
        res += 'subindex=%s, ' % (data[3])
        res += 'data=%s' % str(data[4:])
        return res

    if function_code == '480':
        return 'hello? ' + str(data)

    if function_code[0] in ('1', '2' , '3', '4', '5'):
        if function_code[1:] == '80':
            return '%s PDO transmit %s' % (function_code[0], data)
        if function_code[1:] == '00':
            return '%s PDO receive %s' % (function_code[0], data)

    return ''

def handle_line(hex_id, length, data):
    binary_id = bin(int(hex_id, 16))[2:].zfill(11)
    data_list = data.split(';')[:length]

    # now render them into hex like function code/COB-ID
    function_code = hex(int(binary_id[:4] + '0000000', 2))[2:].upper().zfill(3)
    cob_id = hex(int(binary_id[4:], 2))[2:].upper().zfill(2)

    interpretation = interpret(function_code, cob_id, length, data_list)

    return binary_id, function_code, cob_id, interpretation


def main(path):
    with open(path, 'r') as f_in:
        out_path = path.rsplit('.', 1)[0] + '_binary_ids.txt'
        with open(out_path, 'w') as f_out:
            for line in f_in:
                m = re.search(matcher, line)
                if not m:
                    # copy all non-matching lines
                    f_out.write(line);
                else:
                    # format id as binary and split into 4+7 bit groups (like CANopen COB-IDs)
                    hex_id = m.group(1)
                    length = int(m.group(2))
                    data   = m.group(3)
                    ts     = m.group(4)
                    rest   = m.group(5)

                    binary_id, function_code, cob_id, interpretation = handle_line(hex_id, length, data)

                    f_out.write('%s,%s,%s,%s,%s -> %s %s -> %s %s -> %s\n' % (
                        hex_id, length, data, ts, rest,
                        binary_id[:4], binary_id[4:],
                        function_code, cob_id,
                        interpretation
                    ))

if __name__ == '__main__':
    if len(sys.argv) == 1:
        print("Must supply path")
        sys.exit(1)

    elif len(sys.argv) == 2:
        main(sys.argv[1])

    elif len(sys.argv) == 4:
        hex_id = sys.argv[1]
        length = int(sys.argv[2])
        data = sys.argv[3]

        binary_id, function_code, cob_id, interpretation = handle_line(hex_id, length, data)

        print('%s %s -> %s %s -> %s' % (
             binary_id[:4], binary_id[4:],
            function_code, cob_id,
            interpretation
        ))

