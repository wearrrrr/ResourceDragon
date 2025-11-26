#![feature(str_from_utf16_endian)]

use byteorder::{LittleEndian, ReadBytesExt};
use std::{
    ffi::{CStr, CString},
    io::{Cursor, Read, Seek},
};

use crate::api::{HOST_API, RdFormat};

mod api;

struct NemeaEntry {
    pub name: CString,
    pub offset: u32,
    pub size: u32,
}
struct NemeaArchive {
    pub cur: Cursor<Vec<u8>>,
    pub entries: Vec<NemeaEntry>,
}

impl<'a: 'static> RdFormat<'a> for NemeaArchive {
    fn plugin_init() -> bool {
        HOST_API.log("[NFA0] Nemea NFA0 plugin initialized");
        true
    }
    fn plugin_shutdown() {}

    fn tag() -> &'static CStr {
        c"NFA0"
    }
    fn description() -> &'static CStr {
        c"NFA0 format as seen in 不思議の幻想郷CHRONICLE -クロニクル-"
    }

    fn try_open(buffer: &[u8], _file_name: &CStr) -> Option<Self> {
        let mut cur = Cursor::new(Vec::from(buffer));

        let mut magic = [0u8; 4];
        cur.read_exact(&mut magic).ok()?;
        if &magic != b"NFA0" {
            return None;
        }
        if cur.read_u32::<LittleEndian>().ok()? != 1 {
            return None;
        }
        let file_count = cur.read_u32::<LittleEndian>().ok()?;
        if cur.read_u32::<LittleEndian>().ok()? != 1 {
            return None;
        }

        let mut entries = Vec::with_capacity(file_count as usize);
        for _ in 0..file_count {
            cur.seek_relative(8).ok()?; // checksum
            let size = cur.read_u32::<LittleEndian>().ok()? ^ 0x08080808;
            let offset = cur.read_u32::<LittleEndian>().ok()? ^ 0x08080808;
            cur.seek_relative(4).ok()?; // unknown

            // Needs multiple passes. Gross...
            let mut name_bytes = [0u8; 128];
            cur.read_exact(&mut name_bytes).ok()?;
            for i in 0..128 {
                name_bytes[i] ^= 0x08;
            }

            let terminator_idx = {
                let mut ret: usize = 0;
                for i in (0..128).step_by(2) {
                    if name_bytes[i] == 0 && name_bytes[i + 1] == 0 {
                        break;
                    }
                    ret += 2;
                }
                ret
            };

            let name = String::from_utf16le_lossy(&name_bytes[..terminator_idx]).replace('\\', "/");
            let name = CString::new(name).ok()?;

            entries.push(NemeaEntry {
                name,
                offset,
                size,
            });
        }

        Some(NemeaArchive { cur, entries })
    }
    fn can_handle_file(buffer: &[u8], ext: &CStr) -> bool {
        ext == c"bin" && buffer.len() >= 16 && &buffer[0..4] == b"NFA0"
    }

    fn get_entry_count(&'a self) -> usize {
        self.entries.len()
    }
    fn get_entry_name(&'a self, idx: usize) -> Option<&'a CStr> {
        if idx >= self.entries.len() {
            return None;
        }

        Some(self.entries[idx].name.as_c_str())
    }
    fn get_entry_size(&'a self, idx: usize) -> usize {
        if idx >= self.entries.len() {
            return 0;
        }

        self.entries[idx].size as usize
    }
    fn open_stream(&'a mut self, idx: usize) -> Option<Vec<u8>> {
        if idx >= self.entries.len() {
            return None;
        }
        
        let entry = &self.entries[idx];
        self.cur.seek(std::io::SeekFrom::Start(entry.offset as u64))
            .ok()?;
        let mut content = vec![0u8; entry.size as usize];
        self.cur.read_exact(&mut content).ok()?;
        for i in 0..content.len() {
            content[i] ^= 0x08;
        }

        Some(content)
    }
}

register_plugin!(c"Nemea NFA0", c"1.0.0", NemeaArchive);
