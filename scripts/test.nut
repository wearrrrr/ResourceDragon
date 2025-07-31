archive_format <- {
    sig = 0x90909090
    tag = "SqTestFormat",
    description = "Squirrel Test format -- Does nothing!",

    function canHandleFile(buffer, size, ext) {

        local bytes = read_bytes(buffer, 0, 4);
        local magic = bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);
        return magic == sig;
    }

    function tryOpen(buffer, size, name) {
        return {
            entries = [
                {
                    name = "text"
                    size = 0x0B
                    offset = 0x5
                }
            ]
        };
    }
}
