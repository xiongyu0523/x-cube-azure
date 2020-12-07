# Copyright(c) 2018 STMicroelectronics International N.V.
# Copyright 2017 Linaro Limited
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
import keys
import sys
import argparse
import os
import hashlib
from elftools.elf.elffile import ELFFile
from struct import pack

def gen_ecdsa_p256(args):
    keys.ECDSA256P1.generate().export_private(args.key)

def gen_aes_gcm(args):
    keys.AES_GCM.generate().export_private(args.key)

def gen_aes_cbc(args):
    keys.AES_CBC.generate().export_private(args.key)


keygens = {
        'aes-gcm': gen_aes_gcm,
        'aes-cbc': gen_aes_cbc,
        'ecdsa-p256': gen_ecdsa_p256,
        }

def do_keygen(args):
    if args.type not in keygens:
        msg = "Unexpected key type: {}".format(args.type)
        raise argparse.ArgumentTypeError(msg)
    keygens[args.type](args)

def do_trans(args):
    key = keys.load(args.key)
    end = False
    if args.end:
        end = True
    if args.assembly=="ARM" or args.assembly=="IAR" or args.assembly=="GNU":
        if args.version=="V6M" or args.version=="V7M": 
          out = key.trans(args.section, args.function, end, args.assembly, args.version)
          print (str(out))
        else:
          print ("-v option : Cortex M architecture not supported")
          exit(1)
    else:
        print ("-a option : assembly option not supported")
        exit(1)

def do_getpub(args):
    key = keys.load(args.key)
    key.emit_c()

def do_sign(args):
    payload = []
    with open(args.infile, 'rb') as f:
        payload = f.read()
    # Removing the padding because the hashlib pads properly (512 bit alignment) for SHA256
    # payload = pad(payload)
    key = keys.load(args.key) if args.key else None
    if key.has_sign():
        if key.has_nonce():
            nonce = []
            try:
                with open(args.nonce, 'rb') as f:
                    nonce = f.read()
            except:
                nonce= []
            encrytpted, signature , nonce_used = key.encrypt( payload, nonce)
            if nonce !=nonce_used:
                try:
                    f = open(args.nonce, 'wb')
                    f.write(nonce_used)
                    f.close()
                except:
                    print("nonce filename required")
                    exit(1)
        else:
            signature  = key.sign(payload)
        f = open(args.outfile,"wb")
        f.write(signature)
        f.close()
    else:
        print("Provided Key is not useable to sign ")
        exit(1)


def do_sha(args):
    payload = []
    with open(args.infile, 'rb') as f:
        payload = f.read()
    m = hashlib.sha256()
    buffer=payload
    m.update(buffer)
    signature = m.digest()
    f = open(args.outfile, "wb")
    f.write(signature)
    f.close()

def do_encrypt(args):
    payload = []
    with open(args.infile, 'rb') as f:
        payload = f.read()
        f.close()
    key = keys.load(args.key) if args.key else None
    if key.has_encrypt():
        if key.has_nonce():
            nonce = []
            if args.nonce and args.iv:
                print("either IV or Nonce Required for this key!!!")
                exit(1)
            if args.nonce:
                iv_nonce=args.nonce
            elif args.iv:
                iv_nonce=args.iv
            else:
                print("either IV or Nonce Required for this key!!!")
                exit(1)
            if os.path.isfile(iv_nonce):
                with open(iv_nonce, 'rb') as f:
                    nonce = f.read()
            encrypted , signature , nonce_used = key.encrypt(payload, nonce)
            if nonce !=nonce_used:
                f = open(iv_nonce, 'wb')
                f.write(nonce_used)
                f.close()
        else:
            encrypted ,signature = key.encrypt(payload)

        f=open(args.outfile,"wb")
        f.write(encrypted)
        f.close()
    else:
        print("Key does not support encrypt")
        exit(1)

def do_header_lib(args):
    if (os.path.isfile(args.firmware)):
        size = os.path.getsize(args.firmware)
    else:
        print("no fw file")
        exit(1)
    if os.path.isfile(args.tag):
        f=open(args.tag, 'rb')
        tag = f.read()
    key = keys.load(args.key) if args.key else None
    #empty nonce
    nonce = b''
    protocol = args.protocol
    magic = args.magic.encode()
    version = args.version
    reserved = b'\0'*args.reserved
    if args.nonce and args.iv:
        print("either IV or Nonce Required !!!")
        exit(1)
    iv_nonce=""
    if args.nonce:
        iv_nonce=args.nonce
    elif args.iv:
        iv_nonce=args.iv
    if iv_nonce:
        with open(iv_nonce, 'rb') as f:
            nonce = f.read()

    header = pack('<'+str(len(magic))+'sH'+str(len(nonce))+'s'+'HI'+str(len(tag))+'s'+str(args.reserved)+'s',
                    magic, protocol,nonce, version, size, tag, reserved)
    #GCM needs Nonce to sign
    #AES cannot sign
    if key.has_sign():
        if key.has_nonce() and iv_nonce=="":
            print("sign key required nonce, provide nonce")
            exit(1)
        if key.has_nonce():
            if nonce != b'':
                signature , nonce_used = key.sign(header, nonce)
                if nonce_used !=nonce:
                    print("error nonce used differs")
                    exit(1)
            else:
                print("nonce required for this key")
                exit(1)
        else:
            signature = key.sign(header)
        header +=pack(str(len(signature))+'s',signature)
    else:
        print("Provided Key is not useable to sign header")
        exit(1)
    return header, signature

#header is used only to build install header for merge .elf tool
def do_header(args):
    header ,signature  = do_header_lib(args)
    f=open(args.outfile,"wb")
    if len(signature) == 16:
        signature = 2*signature
    elif len(signature) == 64:
        signature = signature[0:32]
    else:
        print("Unexpected signature size : "+str(len(signature))+"!!")
    f.write(header)
    f.write(signature)
    f.write(signature)
    f.write(signature)
    padding = args.offset - (len(header)+ 3*32)
    while (padding != 0):
        f.write(b'\xff')
        padding = padding - 1
    f.close()

def do_conf(args):
    if (os.path.isfile(args.infile)):
        f = open(args.infile)
        for myline in f:
            if args.define in myline:
                myword = myline.split()
                if myword[1] == args.define:
                    print(myword[2])
                    f.close()
                    return
        f.close()
        print("#DEFINE "+args.define+" not found")
        exit(1)

def do_pack(args):
    header,signature = do_header_lib(args)
    f=open(args.outfile,"wb")
    f.write(header)
    if len(header) > args.offset:
        print("error header is larger than offset before binary")
        sys.exit(1)

    #create the string to padd
    tmp = (args.offset-len(header))*b'\xff'
    #write to file
    f.write(tmp)
    #recopy encrypted file
    binary=open(args.firmware,'rb')
    tmp=binary.read()
    f.write(tmp)
    binary.close()
#find lowest sction to fix base address not matching
def find_lowest_section(elffile):
     lowest_addr = 0
     lowest_size = 0
     for s in elffile.iter_sections():
        sh_addr =  s.header['sh_addr'];
        if sh_addr !=0:
            if lowest_addr == 0:
                lowest_addr = sh_addr
            elif sh_addr < lowest_addr:
                lowest_addr = sh_addr
                lowest_size = s.header['sh_size']
     return lowest_addr, lowest_size

#return base address of segment, and a binary array,
#add padding pattern
def get_binary(elffile,pad=0xff, elftype=0):
    num = elffile.num_segments()
    print("number of segment :"+str(num))
    for i in range(0,num):
        segment= elffile.get_segment(i)
        if segment['p_type'] == 'PT_LOAD':
          if i!=0:
            print(hex(nextaddress))
            if (len(segment.data())):
                padd_size=segment.__getitem__("p_paddr")- nextaddress
                binary+=padd_size*pack("B",pad) 
                binary+=segment.data()
                nextaddress = segment.__getitem__("p_paddr") + len(segment.data())
          else:
            binary=segment.data()
            if elftype == 0:
              base_address =  segment.__getitem__("p_paddr")
            else:
              base_address , lowest_size =  find_lowest_section(elffile)
              offset = base_address - segment.__getitem__("p_paddr")
              binary = binary[offset:]
            nextaddress = base_address + len(binary)
    return binary, base_address

def do_merge(args):
    #get the different element to compute the big binary
    with open(args.infile, 'rb') as f:
        # get the data
        my_elffile = ELFFile(f)
        appli_binary, appli_base = get_binary(my_elffile, args.value,args.elf)
    with open(args.sbsfu, 'rb') as f:
        my_elffile = ELFFile(f)
        sbsfu_binary, sbsfu_base = get_binary(my_elffile, args.value, args.elf)
    with open(args.install, 'rb') as f:
        header_binary = f.read()
    #merge the three elements and padd in between , add some extra byte to
    #appli for aes cbc support
    address_just_after_sbsfu = len(sbsfu_binary)+sbsfu_base
    beginaddress_header = appli_base - len(header_binary)
    #check that header can be put in between sbsfu and appli
    if (beginaddress_header >= address_just_after_sbsfu):
        print("Merging")
        print("SBSFU Base = "+hex(sbsfu_base))
        print("Writing header = "+hex(beginaddress_header))
        print("APPLI Base = "+hex(appli_base))
        padd_before_header =   beginaddress_header - address_just_after_sbsfu
        big_binary = sbsfu_binary + padd_before_header * pack("B",args.value) + header_binary + appli_binary
    else:
        print("sbsfu is too large to merge with appli !!")
        exit(1)
    print("Writing to "+str(args.outfile)+" "+str(len(big_binary)))
    with open(args.outfile, 'wb') as f:
        f.write(big_binary)

subcmds = {
        'keygen': do_keygen,
        'trans':do_trans,
        'getpub': do_getpub,
        #sign a binary with a givent key
        # nonce is required for aes gcm  , if nonce not existing the file is
        # created
        # -k keyfilename -n nonce file
        'sign': do_sign,
        #hash a file with sha256 and provide result in in a file
        'sha256': do_sha,
        #return define value from crpto configuration from .h file
        #define to search default SECBOOT_CRYPTO_SCHEME, -d
        'conf': do_conf,
        #encrypt binary file with provided key
        # -k -n
        'enc': do_encrypt,
        # give what to put  in header and provide the key to compute hmac
        # magic (4 bytes) required, -m
        # protocol version(2 bytes) required , -p
        # nonce optional , -n
        # fwversion (required) 2 bytes, -ver
        # fw file (to get the size)
        # fw tag  (file)
        # reserved size
        # key
        # offset default 512
        'header':do_header,
        # give what to pack a single file header
        # magic (4 bytes) required, -m
        # protocol version(2 bytes) required , -p
        # nonce optional , -n
        # fwversion (required) 2 bytes, -ver
        # fw file (to get the size)
        # fw tag  (file)
        # reserved size
        # key
        # offset default 512
        #
        'pack':do_pack,
        #merge appli.elf , header binary and sbsfu elf in a big binary
        #input file appli.elf
        #-h header file
        #-s sbsfu.elf
        #output file binary to merge
        #-v byte pattern to fill between the different segment default 0xff
        #-p padding length to add to appli binary
        'merge':do_merge
        }

def args():
    parser = argparse.ArgumentParser()
    subs = parser.add_subparsers(help='subcommand help', dest='subcmd')
    keygenp = subs.add_parser('keygen', help='Generate pub/private keypair')
    keygenp.add_argument('-k', '--key', metavar='filename', required=True)
    keygenp.add_argument('-t', '--type', metavar='type',
            choices=['aes-gcm', 'ecdsa-p256','aes-cbc'],
            required=True)
    trans =  subs.add_parser('trans', help='translate key to execute only code')
    trans.add_argument('-k', '--key', metavar='filename', required=True)
    trans.add_argument('-f', '--function', type=str, required = True)
    trans.add_argument('-s', '--section', type=str, default="")
    trans.add_argument('-a', '--assembly',help='fix assembly type IAR or ARM or GNU', type=str,required = False, default="IAR")
    trans.add_argument('-v', '--version',help='fix CORTEX M architecture', type=str,required = False, default="V7M")   
    trans.add_argument('-e', '--end')

    getpub = subs.add_parser('getpub', help='Get public key from keypair')
    getpub.add_argument('-k', '--key', metavar='filename', required=True)

    sign = subs.add_parser('sign', help='Sign an image with a private key')
    sign.add_argument('-k', '--key', metavar='filename', required = True)
    sign.add_argument('-n', '--nonce', metavar='filename', required = False)
    sign.add_argument("infile")
    sign.add_argument("outfile")
    sha = subs.add_parser('sha256', help='hash a file with sha256')
    sha.add_argument("infile")
    sha.add_argument("outfile")
    sha.add_argument('-p', '--padding',type=int, required = False, default=0, help='pad to be a multiple of the given size if needed')
    config = subs.add_parser('conf', help='get cryto config from .h file')
    config.add_argument('-d', '--define', type=str, default='SECBOOT_CRYPTO_SCHEME')
    config.add_argument("infile")
    enc = subs.add_parser('enc', help='encrypt an image with a private key')
    enc.add_argument('-k', '--key', metavar='filename', required = True)
    enc.add_argument('-n', '--nonce', metavar='filename')
    enc.add_argument('-i', '--iv', metavar='filename')
    enc.add_argument("infile")
    enc.add_argument("outfile")

    head = subs.add_parser('header', help='build  installed header file and compute mac according to provided key')
    head.add_argument('-k', '--key', metavar='filename', required = True)
    head.add_argument('-n', '--nonce', metavar='filename')
    head.add_argument('-i', '--iv', metavar='filename')
    head.add_argument('-f', '--firmware', metavar='filename', required = True)
    head.add_argument('-t', '--tag', metavar='filename', required = True)
    head.add_argument('-v', '--version',type=int, required = True)
    head.add_argument('-m', '--magic',type=str, default="SFUM")
    head.add_argument('-p', '--protocol',type=int,  default = 0x1)
    head.add_argument('-r', '--reserved',type=int, default=8)
    head.add_argument('-o', '--offset', type=int, default = 512, required = False)
    head.add_argument("outfile")
    pack = subs.add_parser('pack', help='build header file and compute mac according to key provided')
    pack.add_argument('-k', '--key', metavar='filename', required = True)
    pack.add_argument('-n', '--nonce', metavar='filename')
    pack.add_argument('-i', '--iv', metavar='filename')
    pack.add_argument('-f', '--firmware', metavar='filename', required = True)
    pack.add_argument('-t', '--tag', metavar='filename', required = True)
    pack.add_argument('-v', '--version',type=int, required = True)
    pack.add_argument('-m', '--magic',type=str, default="SFUM")
    pack.add_argument('-p', '--protocol',type=int, default = 0x1)
    pack.add_argument('-r', '--reserved',type=int, default=8)
    pack.add_argument('-o', '--offset', help='offset between start of header and binary', type=int, default=512)
    pack.add_argument('-e', '--elf', help='elf type set to 1 for GNU, 0 for other by default', type=int, default=1) 
    pack.add_argument("outfile")

    mrg = subs.add_parser('merge', help='merge elf appli , install header and sbsfu.elf in a contiguous binary')
    mrg.add_argument('-i', '--install', metavar='filename',  help="filename of installed binary header", required = True)
    mrg.add_argument('-s', '--sbsfu', metavar='filename', help="filename of sbsfu elf", required = True)
    mrg.add_argument('-v', '--value', help= "byte padding pattern", required = False, type=int, default=0xff)
    mrg.add_argument('-p', '--padding', help='pad to add to appli binary, a multiple of the given size if needed',type=int, required = False, default=0)
    mrg.add_argument('-e', '--elf', help='elf type set to 1 for GNU, 0 for other by default', type=int, default=1) 
    mrg.add_argument("infile", help="filename of appli elf file" )
    mrg.add_argument("outfile", help = "filename of contiguous binary")


    args = parser.parse_args()
    if args.subcmd is None:
        print('Must specify a subcommand')
        sys.exit(1)
    subcmds[args.subcmd](args)

if __name__ == '__main__':
    args()

