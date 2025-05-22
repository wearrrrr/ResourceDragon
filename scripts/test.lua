local test_magic = "\x90\x90\x90\x90"

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
    magic_matches = buffer:sub(1, #test_magic) == test_magic;
    print(magic_matches);
    return magic_matches;
end

function RD__TryOpen()
    return false;
end

function RD__GetTag()
    return "LUA_TEST"
end
