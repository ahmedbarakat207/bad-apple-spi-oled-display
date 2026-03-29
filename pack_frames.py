from PIL import Image
import os, pathlib

frames_dir = "frames"
pathlib.Path("data").mkdir(exist_ok=True)

out = open("data/video.bin", "wb")
count = 0

for fname in sorted(os.listdir(frames_dir)):
    if not fname.endswith(".png"):
        continue

    img = Image.open(f"{frames_dir}/{fname}") \
               .convert("L") \
               .resize((128, 64)) \
               .point(lambda x: 255 if x > 128 else 0, "1")

    pixels = img.load()
    buf = bytearray(1024)

    for x in range(128):
        for page in range(8):
            byte = 0
            for bit in range(8):
                y = page * 8 + bit
                if pixels[x, y] == 255:
                    byte |= (1 << bit)
            buf[page * 128 + x] = byte

    out.write(buf)
    count += 1

out.close()
print(f"Done: {count} frames, {count * 1024} bytes")
