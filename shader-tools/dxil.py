"""Small ctypes adapter to Microsoft's assembler and validator (no game code)."""
import ctypes as C
import os
import uuid
from environment import DXC_DIR

P=C.c_void_p
U=C.c_uint32
H=C.c_long
DIR=DXC_DIR
_search=os.add_dll_directory(str(DIR))
dll=C.WinDLL(str(DIR/'dxcompiler.dll'))
dll.DxcCreateInstance.argtypes=[P,P,C.POINTER(P)]
dll.DxcCreateInstance.restype=H

def guid(value):
    return C.create_string_buffer(uuid.UUID(value).bytes_le)

def create(clsid,iid):
    out=P()
    check(dll.DxcCreateInstance(guid(clsid),guid(iid),C.byref(out)))
    return out

def method(obj,index,result,*args):
    table=C.cast(obj,C.POINTER(C.POINTER(P))).contents
    return C.WINFUNCTYPE(result,P,*args)(table[index])

def check(hr):
    if hr<0:raise RuntimeError(f'DXC HRESULT 0x{hr&0xffffffff:08x}')

def release(obj):
    if obj:method(obj,2,U)(obj)

def data(blob):
    if not blob:return b''
    ptr=method(blob,3,P)(blob)
    size=method(blob,4,C.c_size_t)(blob)
    return C.string_at(ptr,size)

def result(obj):
    status=H()
    check(method(obj,3,H,C.POINTER(H))(obj,C.byref(status)))
    error=P()
    method(obj,5,H,C.POINTER(P))(obj,C.byref(error))
    message=data(error).decode('utf-8',errors='replace').rstrip('\0')
    release(error)
    if status.value<0:
        release(obj)
        raise RuntimeError(message or f'DXC status {status.value}')
    blob=P()
    check(method(obj,4,H,C.POINTER(P))(obj,C.byref(blob)))
    release(obj)
    return blob

library=create('6245d6af-66e0-48fd-80b4-4d271796748c','e5204dc7-d18c-4c3c-bdfb-851673980fe7')
assembler=create('d728db68-f903-4f80-94cd-dccf76ec7151','091f7a26-1c1f-4948-904b-e6e3a8a771d5')
validator=create('8ca3e215-f728-4cf3-8cdd-88af917587a1','a6e82bd2-1fd7-4826-9811-2857e797f49a')

def assemble(text):
    raw=text.encode('utf-8')
    buffer=C.create_string_buffer(raw)
    source=P()
    check(method(library,6,H,P,U,U,C.POINTER(P))(library,buffer,len(raw),65001,C.byref(source)))
    operation=P()
    check(method(assembler,3,H,P,C.POINTER(P))(assembler,source,C.byref(operation)))
    try:compiled=result(operation)
    finally:release(source)
    validation=P()
    check(method(validator,3,H,P,U,C.POINTER(P))(validator,compiled,1,C.byref(validation)))
    try:
        validated=result(validation)
        release(validated)
        return data(compiled)
    finally:release(compiled)

