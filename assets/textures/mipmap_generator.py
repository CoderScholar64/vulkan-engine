from PIL import Image
import subprocess
import argparse

parser = argparse.ArgumentParser(description='Convert an image to mipmap textures.')

parser.add_argument('--input', type=str, nargs=1,
                    help='Filename of an image to covert to a bunch of mipmaps!', required=True)

parser.add_argument('--output', type=str, nargs=1,
                    help='Filename of the mipmaps you want to generator', required=True)

parser.add_argument('--encoding', type=str, nargs=1,
                    help='The format that would be exported', default="qoi")

args = parser.parse_args()

im = Image.open(args.input[0])

width = im.size[0]
height = im.size[1]

scale = max(width, height)

index = 0

while scale != 0:
    subprocess.run(["magick", args.input[0], '-resize', '{}x{}'.format(width, height), "{}_{}.{}".format(args.output[0], index, args.encoding)] )
    index = index + 1

    width  =  int(width / 2)
    height = int(height / 2)
    scale  =  int(scale / 2)
