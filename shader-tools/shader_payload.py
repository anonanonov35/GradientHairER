"""Small lossless LZNT1 payloads using the same Windows decoder as the DLL."""
import ctypes as C
import struct

nt=C.WinDLL('ntdll')
nt.RtlGetCompressionWorkSpaceSize.argtypes=[C.c_ushort,C.POINTER(C.c_ulong),C.POINTER(C.c_ulong)]
nt.RtlGetCompressionWorkSpaceSize.restype=C.c_long
nt.RtlCompressBuffer.argtypes=[C.c_ushort,C.c_void_p,C.c_ulong,C.c_void_p,C.c_ulong,C.c_ulong,C.POINTER(C.c_ulong),C.c_void_p]
nt.RtlCompressBuffer.restype=C.c_long
nt.RtlDecompressBuffer.argtypes=[C.c_ushort,C.c_void_p,C.c_ulong,C.c_void_p,C.c_ulong,C.POINTER(C.c_ulong)]
nt.RtlDecompressBuffer.restype=C.c_long

def unpack(payload):
    magic,size,compressed=struct.unpack_from('<4sII',payload)
    assert magic==b'HGZ1' and 64<size<4*1024*1024 and 0<compressed<=size
    assert len(payload)==12+compressed
    output=C.create_string_buffer(size);final=C.c_ulong()
    status=nt.RtlDecompressBuffer(2,output,size,payload[12:],compressed,C.byref(final))
    assert status>=0 and final.value==size
    raw=output.raw
    assert raw.startswith(b'BND4')
    return raw

def pack(raw):
    assert raw.startswith(b'BND4')
    workspace_size=C.c_ulong();fragment_size=C.c_ulong()
    assert nt.RtlGetCompressionWorkSpaceSize(0x102,C.byref(workspace_size),C.byref(fragment_size))>=0
    workspace=C.create_string_buffer(workspace_size.value)
    output=C.create_string_buffer(len(raw)+4096);final=C.c_ulong()
    status=nt.RtlCompressBuffer(0x102,raw,len(raw),output,len(output),4096,C.byref(final),workspace)
    assert status>=0
    payload=struct.pack('<4sII',b'HGZ1',len(raw),final.value)+output.raw[:final.value]
    assert unpack(payload)==raw
    return payload
