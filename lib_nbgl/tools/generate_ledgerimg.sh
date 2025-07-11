# Generate a ledgerimg from any input image file
#
# sh tools/generate_ledgerimg.sh <IMAGE_FILE> [<PRODUCT>]
#
# where:
#  - <FILE_TO_COMPRESS> is the path to the input image file
#  - <PRODUCT> can be --stax, --flex or --apex.
#
# Example for Stax:
#
# ```
# sh tools/generate_ledgerimg.sh ~/Downloads/cryptopunk.png
# ```
# Example for Flex:
#
# ```
# sh tools/generate_ledgerimg.sh ~/Downloads/cryptopunk.png --flex
# ```
#
# Example for Apex:
#
# ```
# sh tools/generate_ledgerimg.sh ~/Downloads/cryptopunk.png --apex
# ```

## Product detection for screen height & width
if [ "$2" = "--flex" ]; then
    FRONT_SCREEN_WIDTH=480
    FRONT_SCREEN_HEIGHT=600
    IMAGE_BPP=4
elif [ "$2" = "--stax" ]; then
    FRONT_SCREEN_WIDTH=400
    FRONT_SCREEN_HEIGHT=672
    IMAGE_BPP=4
elif [ "$2" = "--apex" ]; then
    FRONT_SCREEN_WIDTH=300
    FRONT_SCREEN_HEIGHT=400
    IMAGE_BPP=1
else
    echo "Invalid option. Please use --flex, --stax, or --apex."
    exit 1
fi

# Path to bmp2display python script
BMP2DISPLAY_PY=$BOLOS_NG/public_sdk/lib_nbgl/tools/bmp2display.py

# Input file is the first argument
FILE_INPUT=$1

## Generate a 24 bits BMP file
FRONT_SCREEN_DIM=$FRONT_SCREEN_WIDTH"x"$FRONT_SCREEN_HEIGHT
FILE_OUTPUT_BMP=${FILE_INPUT%.*}.bmp
convert $FILE_INPUT -resize $FRONT_SCREEN_DIM -background white -compose Copy -gravity center -type truecolor -extent $FRONT_SCREEN_DIM $FILE_OUTPUT_BMP
echo "Write" $FILE_OUTPUT_BMP

## Convert BMP file into ledgerimg
FILE_OUTPUT_LEDGERIMG=${FILE_INPUT%.*}.ledgerimg
python3 $BMP2DISPLAY_PY --file --compress --input $FILE_OUTPUT_BMP --bpp $IMAGE_BPP --outfile $FILE_OUTPUT_LEDGERIMG
