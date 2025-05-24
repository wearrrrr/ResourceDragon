signature = 0x90909090

local function hexdump(s)
    for i = 1, #s do
        io.write(string.format("%02X ", s:byte(i)))
        if i % 16 == 0 then io.write("\n") end
    end
    io.write("\n")
end

function register()
    return 0;
end;

function RD__CanHandleFile(buffer, size, ext)
    print("test.lua: RD__CanHandleFile!")
    local magic = string.unpack("<I4", buffer);
    return magic == signature;
end

function RD__TryOpen()
    return false;
end

function RD__GetTag()
    return "LUA_TEST"
end
