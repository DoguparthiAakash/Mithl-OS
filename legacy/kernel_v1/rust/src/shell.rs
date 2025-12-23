use core::slice;
use core::str;
extern crate alloc;
use alloc::vec::Vec;

// Import C functions
extern "C" {
    fn ata_read_sector(lba: u32, buffer: *mut u8) -> i32;
    fn fs_list_files(buffer: *mut u8, size: i32) -> i32;
    fn fs_read_file(name: *const u8, buffer: *mut u8, size: i32) -> i32;
}

#[no_mangle]
pub extern "C" fn rust_handle_command(input_ptr: *const u8, input_len: usize, output_buffer: *mut u8, max_output: usize) -> usize {
    let input_slice = unsafe { slice::from_raw_parts(input_ptr, input_len) };
    let input_str = match str::from_utf8(input_slice) {
        Ok(s) => s.trim(),
        Err(_) => return 0,
    };

    let mut response_buf = [0u8; 2048]; // Increased buffer for ls output
    let mut response_len = 0;

    let parts: Vec<&str> = input_str.split_whitespace().collect();
    if parts.is_empty() {
        return 0;
    }
    let cmd = parts[0];

    if cmd == "help" {
        let msg = "Available commands:\n  ls    - List files\n  cat   - Show file content\n  whoami- Current user\n  date  - System date\n  clear - Clear screen\n  echo  - Echo text\n  os    - OS Info\n  read  - Read Sector 0";
        response_len = copy_str(msg, &mut response_buf);
    } else if cmd == "clear" {
        response_len = 0; 
    } else if cmd == "ls" {
        // Call C function to list files
        unsafe {
            let ret = fs_list_files(response_buf.as_mut_ptr(), response_buf.len() as i32);
            if ret >= 0 {
                response_len = ret as usize;
            } else {
                response_len = copy_str("Failed to list files.", &mut response_buf);
            }
        }
    } else if cmd == "cat" {
        if parts.len() < 2 {
            response_len = copy_str("Usage: cat <filename>", &mut response_buf);
        } else {
            let filename = parts[1];
            // Null-terminate filename
            let mut name_buf = [0u8; 64];
            if filename.len() < 63 {
                name_buf[..filename.len()].copy_from_slice(filename.as_bytes());
                name_buf[filename.len()] = 0;
                
                unsafe {
                    let ret = fs_read_file(name_buf.as_ptr(), response_buf.as_mut_ptr(), response_buf.len() as i32);
                    if ret >= 0 {
                        response_len = ret as usize;
                    } else {
                        response_len = copy_str("File not found or read error.", &mut response_buf);
                    }
                }
            } else {
                response_len = copy_str("Filename too long.", &mut response_buf);
            }
        }
    } else if cmd == "whoami" {
        response_len = copy_str("root", &mut response_buf);
    } else if cmd == "date" {
        // Placeholder until RTC driver is bound
        response_len = copy_str("Sat Dec 14 01:15:00 UTC 2025", &mut response_buf);
    } else if cmd == "echo" {
        if parts.len() > 1 {
             // Reconstruct string logic roughly, or just assume input_str subslicing
             let echo_content = &input_str[5..]; // "echo " is 5 chars
             response_len = copy_str(echo_content, &mut response_buf);
        }
    } else if cmd == "os" {
        response_len = copy_str("Mithl OS v0.4 (Rust Shell Enabled)", &mut response_buf);
    } else if cmd == "read" {
        let mut sector_buf = [0u8; 512];
        let ret = unsafe { ata_read_sector(0, sector_buf.as_mut_ptr()) };
        
        if ret == 0 {
            if sector_buf[510] == 0x55 && sector_buf[511] == 0xAA {
                response_len = copy_str("Read Sector 0 Success! Found Boot Signature (0x55AA).", &mut response_buf);
            } else {
                response_len = copy_str("Read Sector 0 Success! (No Boot Sig)", &mut response_buf);
            }
        } else {
            response_len = copy_str("ATA Read Failed (Error Bit Set)", &mut response_buf);
        }
    } else {
        response_len = copy_str("Unknown command. Type 'help' for list.", &mut response_buf);
    }

    if response_len > max_output {
        response_len = max_output;
    }
    
    unsafe {
        let dest = slice::from_raw_parts_mut(output_buffer, max_output);
        if response_len > 0 {
            dest[..response_len].copy_from_slice(&response_buf[..response_len]);
        }
        // Ensure null termination if possible or rely on size return
        if response_len < max_output {
            dest[response_len] = 0;
        }
    }

    return response_len;
}

fn copy_str(src: &str, dest: &mut [u8]) -> usize {
    let bytes = src.as_bytes();
    let len = if bytes.len() > dest.len() { dest.len() } else { bytes.len() };
    dest[..len].copy_from_slice(&bytes[..len]);
    len
}
