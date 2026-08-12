"""!
@file ttf2gfx.py
@brief Converts TTF/OTF fonts into Adafruit GFX compatible C header files.
@details Processes a batch of fonts and generates a single header file containing
         the PROGMEM bitmap arrays, glyph structures, and font metadata.
"""

import sys
import freetype

if len(sys.argv) < 5 or (len(sys.argv) - 2) % 3 != 0:
    print("Usage: python ttf2gfx.py <output_file.h> <font1.otf> <size1> <name1> ...")
    sys.exit(1)

output_file = sys.argv[1]

fonts_to_process = []
args = sys.argv[2:]
for i in range(0, len(args), 3):
    fonts_to_process.append((args[i], int(args[i+1]), args[i+2]))

first_char = 32
## Extracts up to Extended ASCII (255) instead of standard ASCII (127) 
## to generate bitmaps for Spanish diacritics (á, é, í, ó, ú) and 'ñ'.
last_char = 255

with open(output_file, "w", encoding="utf-8") as f:
    f.write("#pragma once\n")
    f.write("#include <stdint.h>\n")
    f.write("#ifndef PROGMEM\n#define PROGMEM\n#endif\n\n")

    for ttf_path, size_pt, font_name in fonts_to_process:
        print(f"Processing '{ttf_path}' at size {size_pt}pt as '{font_name}'...")
        
        try:
            face = freetype.Face(ttf_path)
            ## Hardcoded 141 DPI matches Adafruit's original font rasterization baseline 
            ## to ensure physical size parity with legacy fonts.
            face.set_char_size(size_pt * 64, 0, 141, 0)
        except Exception as e:
            print(f" ERROR opening {ttf_path}: {e}. Skipping font.")
            continue
        
        bitmap_array = []
        glyphs = []
        bitmap_offset = 0
        
        for c in range(first_char, last_char + 1):
            width = 0
            height = 0
            xAdvance = 0
            xOffset = 0
            yOffset = 0
            bits = []

            try:
                ## FT_LOAD_TARGET_MONO is strictly required to align the rasterizer 
                ## with Adafruit GFX's 1-bit-per-pixel memory layout constraint.
                face.load_char(chr(c), freetype.FT_LOAD_RENDER | freetype.FT_LOAD_TARGET_MONO)
                bitmap = face.glyph.bitmap
                width = bitmap.width
                height = bitmap.rows
                pitch = bitmap.pitch
                
                xAdvance = face.glyph.advance.x >> 6
                xOffset = face.glyph.bitmap_left
                yOffset = -face.glyph.bitmap_top
                
                for y in range(height):
                    for x in range(width):
                        byte_idx = y * pitch + (x // 8)
                        bit_idx = 7 - (x % 8)
                        bit = (bitmap.buffer[byte_idx] >> bit_idx) & 1
                        bits.append(bit)
            except Exception:
                ## Suppressing the exception guarantees that missing or corrupted characters 
                ## map to zero-dimension geometric spacers rather than breaking the sequence offset.
                pass
                
            for i in range(0, len(bits), 8):
                b = 0
                for j in range(8):
                    if i + j < len(bits):
                        b |= (bits[i+j] << (7 - j))
                bitmap_array.append(b)
                
            glyphs.append({
                'offset': bitmap_offset,
                'w': width,
                'h': height,
                'xAdv': xAdvance,
                'xOff': xOffset,
                'yOff': yOffset
            })
            
            bitmap_offset += len(bits) // 8 + (1 if len(bits) % 8 != 0 else 0)

        yAdvance = face.size.height >> 6
        
        f.write(f"const uint8_t {font_name}Bitmaps[] PROGMEM = {{\n  ")
        if len(bitmap_array) == 0:
            ## Injecting a dummy byte bypasses compilation halts in strict C/C++ environments 
            ## that forbid zero-length arrays on unrenderable fonts.
            f.write("0x00") 
        else:
            for i, b in enumerate(bitmap_array):
                f.write(f"0x{b:02X}, ")
                if (i + 1) % 12 == 0:
                    f.write("\n  ")
        f.write("\n};\n\n")
        
        f.write(f"const GFXglyph {font_name}Glyphs[] PROGMEM = {{\n")
        for g in glyphs:
            f.write(f"  {{ {g['offset']}, {g['w']}, {g['h']}, {g['xAdv']}, {g['xOff']}, {g['yOff']} }},\n")
        f.write("};\n\n")
        
        f.write(f"const GFXfont {font_name} PROGMEM = {{\n")
        f.write(f"  (uint8_t *){font_name}Bitmaps,\n")
        f.write(f"  (GFXglyph *){font_name}Glyphs,\n")
        f.write(f"  0x{first_char:02X}, 0x{last_char:02X}, {yAdvance}\n")
        f.write("};\n\n")

print(f"\nSuccess! File {output_file} generated with {len(fonts_to_process)} fonts in total.")