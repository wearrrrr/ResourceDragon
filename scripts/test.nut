archive_format <- {
    sig = 0x90909090
    tag = "SqTestFormat",
    description = "Squirrel Test format -- Does nothing!",

    function canHandleFile(buffer, size, ext) {
        if (size < 4) return false;

        local magic = buffer[0] | (buffer[1] << 8) | (buffer[2] << 16) | (buffer[3] << 24);
        // print(format("0x%x", magic));

        return magic == sig;
    }

    function tryOpen(buffer, size, name) {
        return null;
    }
}
