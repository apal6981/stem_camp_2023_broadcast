import serial
import struct
from mido import MidiFile
import mido

# ser = serial.Serial('COM6', 115200, timeout=1)

# if ser is None:
#     exit(1)

def generate_anncoucement_bytes(part: int, part_len: list) -> bytes:
    # return struct.pack("<bbbbbb",((part & 0b11)<< 6) | (part_0_len & 0b1111110000)>>4,((part_0_len & 0b1111)<< 4) | (part_1_len& 0b1111000000)>>4,
    #                               (part_1_len & 0b111111)<<2 | (part_2_len & 0b1100000000)>>8,(part_2_len & 0b11111111),
    #                               (part_3_len & 0b1111111100)>>2,(part_3_len&0b11)<<6)
    return struct.pack("<B"+"H"*len(part_len),part,*part_len)

def generate_song_bytes(part:int, sequence:int,note:int,red:int,green:int,blue:int,duration:int) -> bytes:
    return struct.pack("<BHBBBBH",part,sequence,note,red,green,blue,duration)

def checksum_calc(payload:bytes) -> bytes:
    checksum = 0
    for i in range(0,len(payload),2):
        checksum += struct.unpack("<H",payload[i:i+2])[0]
    return struct.pack("<H",checksum^0xFFFF)

def add_header(p_type: int, num_payload: int, repeat: int) -> bytes:
    return checksum_calc + struct.pack("<B",(p_type & 0b111) << 5 | (num_payload & 0b1111) << 1 | (repeat & 0b1))

def generate_song_packet(part: int, packets: list, repeat: int) -> bytes:
    packet = b""
    for pack in packets:
        packet += generate_song_bytes(part,*pack)
    return add_header(1,len(packets),repeat) + packet
    



if __name__ == "__main__":
    print(generate_anncoucement_bytes(1,[1,1,1,1]))
    # pirates = MidiFile("moonlight.mid",clip=True)
    # print(pirates.length)
    # print(pirates.tracks)
    # print(pirates.ticks_per_beat)
    # for msg in pirates.tracks[1][:100]:
    #     print(msg)
    # for msg in pirates.tracks[1][:20]:
    #     print(msg, msg.type, mido.tick2second(msg.time,pirates.ticks_per_beat,300000))