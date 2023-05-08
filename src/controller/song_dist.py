import serial
import struct
from mido import MidiFile
import mido
import time

ser = serial.Serial('/dev/ttyACM0', 115200, timeout=1)

# if ser is None:
#     exit(1)

def generate_anncoucement_bytes(part: int, part_len: list) -> bytes:
    # return struct.pack("<bbbbbb",((part & 0b11)<< 6) | (part_0_len & 0b1111110000)>>4,((part_0_len & 0b1111)<< 4) | (part_1_len& 0b1111000000)>>4,
    #                               (part_1_len & 0b111111)<<2 | (part_2_len & 0b1100000000)>>8,(part_2_len & 0b11111111),
    #                               (part_3_len & 0b1111111100)>>2,(part_3_len&0b11)<<6)
    return struct.pack("!B"+"H"*len(part_len),part,*part_len)

def generate_song_bytes(part:int, sequence:int,duration:int,red:int,green:int,blue:int,note:int) -> bytes:
    return struct.pack("!HHBBBBB",sequence,duration,part,red,green,blue,note)

def checksum_calc(payload:bytes) -> bytes:
    checksum = 0
    for i in range(0,len(payload),2):
        checksum += struct.unpack("!H",payload[i:i+2])[0]
    print(checksum,~checksum&0xFFFF)
    return struct.pack("!H",~checksum&0xFFFF)

def add_header(p_type: int, num_payload: int, repeat: int) -> bytes:
    return struct.pack("!B",(p_type & 0b111) << 5 | (num_payload & 0b1111) << 1 | (repeat & 0b1))

def generate_song_packet(part: int, packets: list, repeat: int) -> bytes:
    packet = b""
    for pack in packets:
        packet += generate_song_bytes(part,*pack)
    packet = add_header(1,len(packets),repeat) + packet
    if len(packet) % 2:
        packet += struct.pack("!b",0)
    packet = checksum_calc(packet) + packet
    # print(len(packet),packet, checksum_calc(packet) + packet)
    # return checksum_calc(packet) + packet
    return struct.pack("!B",len(packet)) + packet

def generate_anouncement_packet(part: int,part_len: list, repeat: int) -> bytes:
    packet = b""
    packet = add_header(0,1,repeat) + generate_anncoucement_bytes(part, part_len)
    packet = checksum_calc(packet) + packet
    print(len(packet),packet)
    return struct.pack("!B",len(packet)) + packet
    
def generate_other_control_packet(p_type: int, repeat:int) -> bytes:
    packet = add_header(p_type,0,repeat)
    if len(packet) % 2:
        packet += struct.pack("!b",0)
    packet = checksum_calc(packet) + packet
    return struct.pack("!B",len(packet)) + packet


imperial_treble = [[0,490,234, 0, 21,69],
     [1,10,0,0,0,91],
     [2,490,234, 0, 21,69],
     [3,10,0,0,0,91],
     [4,490,234, 0, 21,69],
     [5,10,0,0,0,91],
     [6,365,140, 0, 115,65],
     [7,10,0,0,0,91],
     [8,115,255, 17, 0,72],
     [9,10,0,0,0,91],
     [10,490,234, 0, 21,69],
     [11,10,0,0,0,91],
     [12,365,140, 0, 115,65],
     [13,10,0,0,0,91],
     [14,115,255, 17, 0,72],
     [15,10,0,0,0,91],
     [16,990,234, 0, 21,69],
     [17,10,0,0,0,91],
     [18,490,255, 129, 0,76],
     [19,10,0,0,0,91],
     [20,490,255, 129, 0,76],
     [21,10,0,0,0,91],
     [22,490,255, 129, 0,76],
     [23,10,0,0,0,91],
     [24,365,140, 0, 115,65],
     [25,10,0,0,0,91],
     [26,115,255, 17, 0,72],
     [27,10,0,0,0,91],
     ]


if __name__ == "__main__":
    # ser.write(generate_anouncement_packet(2,[6,6],0))
    # time.sleep(.1)
    # ser.write(generate_anouncement_packet(2,[6,6],1))
    # time.sleep(.1)
    # ser.write(generate_anouncement_packet(2,[6,6],1))
    # time.sleep(.1)
    # for i in range(2):
    #     ser.write(generate_song_packet(i,[[0,1000,255,255,255,60],[1,1000,255,0,0,72],
    #                                       [2,1000,255,255,255,60],[3,1000,255,255,255,60-12],
    #                                       [4,1000,255,0,0,60],[5,1000,255,255,255,60+3]],0))
    #     time.sleep(.1)

    # ser.write(generate_anouncement_packet(1,[28],0))
    # time.sleep(.1)
    # ser.write(generate_anouncement_packet(1,[28],1))
    # time.sleep(.1)
    # ser.write(generate_song_packet(0,imperial_treble[:8],0))
    # time.sleep(.1)
    # ser.write(generate_song_packet(0,imperial_treble[8:16],0))
    # time.sleep(.1)
    # ser.write(generate_song_packet(0,imperial_treble[16:],0))
    # time.sleep(1)
    # ser.write(generate_other_control_packet(2,0))
    # time.sleep(10)
    # ser.write(generate_other_control_packet(3,0))
    # ser.write(generate_other_control_packet(4,0))

    # print(generate_song_packet(0,[[255,255,255,60,0,300]],0))
    # print(generate_anouncement_packet(4,[1,1,1,1],0))
    # ser.write(generate_anncoucement_bytes(1,[1]))
    # print(generate_anncoucement_bytes(1,[1]))
    # time.sleep(1)
    # while True:
    #     print(ser.readline())
        # time.sleep(1)
    # print(generate_anncoucement_bytes(1,[1,1,1,1]))
    # pirates = MidiFile("moonlight.mid",clip=True)
    # print(pirates.length)
    # print(pirates.tracks)
    # print(pirates.ticks_per_beat)
    # for msg in pirates.tracks[1][:100]:
    #     print(msg)
    # for msg in pirates.tracks[1][:20]:
    #     print(msg, msg.type, mido.tick2second(msg.time,pirates.ticks_per_beat,300000))
    dragon = MidiFile("./dragonborn2.mid",clip=True)



    dragon_notes = []
    wait_time = 0
    sequence_number = 0
    start_index = 0
    temp= []
    print(dragon.tracks)
    for msg in dragon.tracks[3][:20]:
        print(msg, msg.type, mido.tick2second(msg.time,dragon.ticks_per_beat,600000))
    for index, msg in enumerate(dragon.tracks[3]):
        if msg.type == "track_name":
            temp = [0,0,0,0,0,112]
            # dragon_notes.append([sequence_number,
            #                      int(round(mido.tick2second(dragon.tracks[3][index+1].time,dragon.ticks_per_beat,600000),3)*1000),
            #                      0,0,0,
            #                      91])
        if msg.type == "note_on":
            temp[1] = wait_time + int(round(mido.tick2second(msg.time,dragon.ticks_per_beat,600000),3)*1000)
            dragon_notes.append(temp)
            temp = [len(dragon_notes),0,0,0,0,msg.note]
        elif msg.type == "note_off":
            temp[1] = wait_time + int(round(mido.tick2second(msg.time,dragon.ticks_per_beat,600000),3)*1000)
            dragon_notes.append(temp)
            temp = [len(dragon_notes),0,0,0,0,112]
            wait_time = 0
        else:
            wait_time += int(round(mido.tick2second(msg.time,dragon.ticks_per_beat,600000),3)*1000)
        # print(msg, msg.type, mido.tick2second(msg.time,dragon.ticks_per_beat,600000))
    # for msg in dragon_notes:
    #     print(msg)

    ser.write(generate_anouncement_packet(1,[len(dragon_notes)],0))
    time.sleep(.1)
    ser.write(generate_anouncement_packet(1,[len(dragon_notes)],1))
    time.sleep(.1)
    for i in range(0,len(dragon_notes),10):
        ser.write(generate_song_packet(0,dragon_notes[i:i+10],0))
        time.sleep(.05)
    # ser.write(generate_song_packet(0,imperial_treble[:8],0))
    # time.sleep(.1)
    # ser.write(generate_song_packet(0,imperial_treble[8:16],0))
    # time.sleep(.1)
    # ser.write(generate_song_packet(0,imperial_treble[16:],0))
    time.sleep(1)
    ser.write(generate_other_control_packet(2,0))
    time.sleep(100)
    ser.write(generate_other_control_packet(3,0))
    ser.write(generate_other_control_packet(4,0))