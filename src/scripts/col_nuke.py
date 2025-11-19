import argparse
import os
from PIL import Image
import numpy as np

def zero_out_channels(input_path, output_path, channels_to_zero):
    """
    Reads an image, zeros out the specified color channels, and saves the result.
    
    Args:
        input_path (str): Path to the input image file.
        output_path (str): Path to save the processed image file.
        channels_to_zero (str): A string containing channel letters (e.g., 'RG', 'B', 'RGBA').
    """
    
    # 1. Open the image
    try:
        img = Image.open(input_path)
    except FileNotFoundError:
        print(f"Error: Input file not found at {input_path}")
        return
    except Exception as e:
        print(f"Error opening image: {e}")
        return

    # Convert to NumPy array for efficient manipulation
    # This also helps standardize the channel order (last dimension)
    img_array = np.array(img, dtype=np.uint8)
    
    # Check if the image has channels (i.e., not grayscale/single channel)
    if img_array.ndim < 3:
        print("Error: Image must have at least 3 channels (RGB) to zero out.")
        return

    # Determine the channel count and channel mapping (0-indexed)
    num_channels = img_array.shape[-1]
    
    # Channel index mapping based on standard Pillow/NumPy arrays (R, G, B, A)
    # This assumes the image is RGB or RGBA.
    channel_map = {
        'R': 0, 
        'G': 1, 
        'B': 2, 
        'A': 3 
    }

    # 2. Zero out the specified channels
    for channel_letter in channels_to_zero.upper():
        if channel_letter in channel_map:
            index = channel_map[channel_letter]
            
            # Check if the requested channel exists in the image
            if index < num_channels:
                # Use slicing to set all pixels in the specified channel to 0
                # img_array[all_rows, all_cols, index_of_channel] = 0
                img_array[:, :, index] = 0
                print(f"Zeroed out channel: {channel_letter}")
            else:
                print(f"Warning: Channel '{channel_letter}' not found in image mode '{img.mode}'. Skipping.")
        else:
            print(f"Warning: Unknown channel '{channel_letter}'. Skipping.")

    # 3. Create a new image from the modified array
    processed_img = Image.fromarray(img_array)

    # 4. Save the processed image in the original format
    try:
        # Extract original format for saving (defaults to JPEG if unknown)
        original_format = img.format if img.format else 'JPEG'
        
        # Determine the final output path
        if not output_path:
            # Construct a new name if no output path is provided
            base, ext = os.path.splitext(input_path)
            # Add a suffix indicating the channels that were removed
            channel_str = "".join(sorted(channels_to_zero.upper()))
            output_path = f"{base}_no{channel_str}{ext}"

        # Pillow saves the image using the inferred format from the filename extension
        processed_img.save(output_path, format=original_format)
        print(f"\nSuccessfully saved processed image to: {output_path}")

    except Exception as e:
        print(f"Error saving image: {e}")


def main():
    parser = argparse.ArgumentParser(
        description="Zero out specified color channels (R, G, B, A) in an image file."
    )
    parser.add_argument(
        "input_file", 
        help="Path to the input image file (e.g., image.png, photo.jpg)."
    )
    parser.add_argument(
        "-c", "--channels", 
        required=True, 
        type=str,
        help="Channels to zero out (e.g., 'R', 'G', 'B', 'A' or combinations like 'RB', 'GA'). Case-insensitive."
    )
    parser.add_argument(
        "-o", "--output_file", 
        type=str, 
        default="", 
        help="Optional path for the output file. If not specified, a suffix is added to the input file name."
    )

    args = parser.parse_args()
    
    # Run the main processing function
    zero_out_channels(args.input_file, args.output_file, args.channels)

if __name__ == "__main__":
    main()