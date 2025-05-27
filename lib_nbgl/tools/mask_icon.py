from pathlib import Path
from PIL import Image
import argparse


def apply_mask(input_path, output_path):
    try:
        # Open images
        in_img = Image.open(input_path)

        # Convert in RGB
        if in_img.mode != 'RGB':
            in_img = in_img.convert('RGB')

        # Obtenir les dimensions de l'image
        width, height = in_img.size
        assert width == 14, "Image width must be 14px"
        assert height == 14, "Image height must be 14px"

        out_img = Image.new('RGB', (width, height))

        current_dir = Path(__file__).parent.resolve()
        mask_img = Image.open(f"{current_dir}/mask_14px.gif")
        if mask_img.mode != 'RGB':
            mask_img = mask_img.convert('RGB')

        # parse pixels
        for y in range(height):
            for x in range(width):
                # invert and apply mask
                pixel = in_img.getpixel((x, y))
                mask_pixel = mask_img.getpixel((x, y))
                if pixel[0] == 0 and mask_pixel[0] == 255:
                    pixel = (255, 255, 255)
                else:
                    pixel = (0, 0, 0)

                out_img.putpixel((x, y), pixel)

        out_img.save(output_path)

    except Exception as e:
        print(f"Error : {e}")

def main():
    parser = argparse.ArgumentParser(
        description='Apply a mask on the given icon and save it at the given path')
    parser.add_argument('--input', help="image to mask", type=str)
    parser.add_argument('--output', help="output icon", type=str)
    args = parser.parse_args()
    apply_mask(args.input, args.output)

if __name__ == "__main__":
    main()
