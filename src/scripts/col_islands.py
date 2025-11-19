import argparse
from PIL import Image
import numpy as np
from tqdm import tqdm
import yaml
from scipy import ndimage

def process_image_to_islands(input_path, output_path, transparent_black=False, shrink_pixels=0, output_yaml=False, draw_centers=False, center_color_str="0,0,0", only_centers=False):
    try:
        print(f"Loading image from: {input_path}")
        img = Image.open(input_path).convert("RGBA") 
        width, height = img.size
        input_array = np.array(img)
        
        # Parse center color string into a NumPy array
        center_color_list = [int(c.strip()) for c in center_color_str.split(',')]
        CENTER_COLOR = np.array(center_color_list, dtype=np.uint8)

        # Output array initialized to WHITE
        output_array = np.full((height, width, 3), 255, dtype=np.uint8)
        
        BLACK = np.array([0, 0, 0], dtype=np.uint8)
        WHITE = np.array([255, 255, 255], dtype=np.uint8)

        def is_fully_transparent(pixel_rgba):
            return pixel_rgba[3] == 0

        def horizontal_pass():
            print("Starting Horizontal Pass (Rows)...")
            for y in tqdm(range(height), desc="Horizontal Pass", unit="row"):
                current_pixel_rgba = input_array[y, 0]
                
                if not is_fully_transparent(current_pixel_rgba):
                    output_array[y, 0] = BLACK 
                else:
                    output_array[y, 0] = BLACK 
                
                for x in range(1, width):
                    next_pixel_rgba = input_array[y, x]
                    
                    if not np.array_equal(next_pixel_rgba[:3], current_pixel_rgba[:3]):
                        
                        if not is_fully_transparent(current_pixel_rgba):
                            output_array[y, x - 1] = BLACK
                        
                        if not is_fully_transparent(next_pixel_rgba):
                            output_array[y, x] = BLACK
                        
                        current_pixel_rgba = next_pixel_rgba
                    
                    if is_fully_transparent(next_pixel_rgba):
                        output_array[y, x] = BLACK
                        
                if not is_fully_transparent(current_pixel_rgba):
                    output_array[y, width - 1] = BLACK


        def vertical_pass():
            print("Starting Vertical Pass (Columns)...")
            for x in tqdm(range(width), desc="Vertical Pass", unit="col"):
                current_pixel_rgba = input_array[0, x]
                
                if not is_fully_transparent(current_pixel_rgba):
                    if not np.array_equal(output_array[0, x], BLACK):
                        output_array[0, x] = BLACK
                else:
                    output_array[0, x] = BLACK
                
                for y in range(1, height):
                    next_pixel_rgba = input_array[y, x]
                    
                    if not np.array_equal(next_pixel_rgba[:3], current_pixel_rgba[:3]):
                        
                        if not is_fully_transparent(current_pixel_rgba):
                            if not np.array_equal(output_array[y - 1, x], BLACK):
                                output_array[y - 1, x] = BLACK
                                
                        if not is_fully_transparent(next_pixel_rgba):
                            if not np.array_equal(output_array[y, x], BLACK):
                                output_array[y, x] = BLACK
                        
                        current_pixel_rgba = next_pixel_rgba
                    
                    if is_fully_transparent(next_pixel_rgba):
                        output_array[y, x] = BLACK
                        
                if not is_fully_transparent(current_pixel_rgba):
                    if not np.array_equal(output_array[height - 1, x], BLACK):
                        output_array[height - 1, x] = BLACK
            
        # --- PHASE 1: Generate and Define Islands (Always Runs to define shapes) ---
        horizontal_pass()
        vertical_pass()
        
        if shrink_pixels > 0:
            print(f"Shrinking islands by {shrink_pixels} pixel(s)...")
            
            for _ in tqdm(range(shrink_pixels), desc="Eroding Islands", unit="iter"):
                island_mask = output_array[:, :, 0] == 255 
                
                padded_mask = np.pad(island_mask, 1, mode='constant', constant_values=False)
                
                eroded_mask = (
                    padded_mask[1:-1, 1:-1] & 
                    padded_mask[:-2, 1:-1] &  
                    padded_mask[2:, 1:-1] &   
                    padded_mask[1:-1, :-2] &  
                    padded_mask[1:-1, 2:]     
                )
                
                output_array[~eroded_mask] = BLACK

        # --- PHASE 2: Island Analysis (Runs on fully defined islands) ---
        should_analyze = output_yaml or draw_centers
        labeled_array = None
        num_features = 0
        centers = []
        
        if should_analyze:
            print("Starting island analysis...")
            # island_mask is created here based on the final, processed islands
            island_mask = output_array[:, :, 0] == 255
            labeled_array, num_features = ndimage.label(island_mask)
            
            if num_features > 0:
                centers = ndimage.center_of_mass(island_mask, labeled_array, range(1, num_features + 1))

        # --- PHASE 3: Check for 'Only Centers' Output & Draw Centers ---
        if only_centers:
            # FIX: If only_centers is true, reset the canvas to WHITE before drawing centers.
            # This preserves the island definition data (centers) but clears the output image.
            print("Outputting only centers: Clearing canvas...")
            output_array[:] = WHITE
        
        # --- Draw Centers Feature ---
        if draw_centers and num_features > 0:
            print(f"Drawing {num_features} island centers with color {center_color_str}...")
            for center_y, center_x in tqdm(centers, desc="Drawing Centers", unit="center"):
                py, px = int(round(center_y)), int(round(center_x))
                
                if 0 <= py < height and 0 <= px < width:
                    output_array[py, px] = CENTER_COLOR


        # --- YAML Output Feature ---
        if output_yaml and num_features > 0:
            print("Starting YAML data generation...")
            
            island_data = {'metadata': {'total_islands': num_features, 'image_dimensions': [height, width]}, 'islands': {}}
            
            sizes = ndimage.sum(island_mask, labeled_array, range(1, num_features + 1))
            slices = ndimage.find_objects(labeled_array)

            for i in tqdm(range(num_features), desc="Generating YAML Data", unit="island"):
                island_id = i + 1
                
                slice_y, slice_x = slices[i]
                min_y, max_y = slice_y.start, slice_y.stop
                min_x, max_x = slice_x.start, slice_x.stop

                center_y, center_x = centers[i]
                
                island_data['islands'][f'island_{island_id}'] = {
                    'id': int(island_id),
                    'area': int(sizes[i]),
                    'bounding_box': {
                        'min_x': int(min_x), 'min_y': int(min_y), 
                        'max_x': int(max_x - 1), 'max_y': int(max_y - 1), 
                        'width': int(max_x - min_x), 'height': int(max_y - min_y)
                    },
                    'center': {
                        'x': float(round(center_x, 2)), 
                        'y': float(round(center_y, 2))
                    }
                }
                
            if output_path.lower().endswith('.png') or output_path.lower().endswith('.jpg'):
                yaml_output_path = output_path.rsplit('.', 1)[0] + '.yaml'
            else:
                yaml_output_path = output_path + '.yaml'

            with open(yaml_output_path, 'w') as f:
                yaml.dump(island_data, f, default_flow_style=False)
            
            print(f"✅ Island data saved to: **{yaml_output_path}**")

        # --- Final Image Saving ---

        if transparent_black:
            print("Creating final image with transparent background...")
            
            height, width = output_array.shape[:2]
            final_rgba = np.zeros((height, width, 4), dtype=np.uint8)
            final_rgba[:, :, :3] = output_array
            
            if only_centers:
                # If ONLY_CENTERS is active, the background is WHITE. We make WHITE pixels transparent.
                is_background = np.all(output_array == WHITE, axis=2)
                final_rgba[:, :, 3] = np.where(is_background, 0, 255)
            else:
                # If ISLANDS are active, the background is BLACK/CENTER_COLOR. We make BLACK pixels transparent.
                is_black = np.all(output_array == BLACK, axis=2)
                final_rgba[:, :, 3] = np.where(is_black, 0, 255)
            
            print(f"Saving final RGBA image to: {output_path}")
            final_img = Image.fromarray(final_rgba, 'RGBA')
            final_img.save(output_path)
            
        else:
            print(f"Saving final RGB image to: {output_path}")
            final_img = Image.fromarray(output_array, 'RGB')
            final_img.save(output_path)
        
        print(f"\n✅ **Processing complete!** Output saved to: **{output_path}**")

    except ImportError as e:
        print(f"\n❌ Missing dependency: {e}. You need to install 'PyYAML' (pip install pyyaml) and 'SciPy' (pip install scipy) to use the advanced features.")
    except FileNotFoundError:
        print(f"\n❌ Error: Input file not found at '{input_path}'.")
    except Exception as e:
        print(f"\n❌ An unexpected error occurred: {e}")

def main():
    parser = argparse.ArgumentParser(
        description="Process an image to create 'islands' based on color transitions (edge detection)."
    )
    
    parser.add_argument(
        "-i", "--input_file", 
        type=str, 
        required=True,
        help="Path to the input image file (e.g., image.png)."
    )
    parser.add_argument(
        "-o", "--output_file", 
        type=str, 
        required=True,
        help="Path for the output image file (e.g., islands_out.png). Use .png for transparent output."
    )
    
    parser.add_argument(
        "-t", "--transparent_black",
        action="store_true",
        help="If set, the background (white if --only_centers, or black otherwise) will be made fully transparent."
    )

    parser.add_argument(
        "-s", "--shrink_pixels",
        type=int,
        default=0,
        help="Amount of pixels (N) to shrink the white island regions by (morphological erosion)."
    )

    parser.add_argument(
        "-y", "--output_yaml",
        action="store_true",
        help="If set, a final pass calculates data for all white islands (bounding boxes, centers, area) and saves it to a YAML file."
    )

    parser.add_argument(
        "-c", "--draw_centers",
        action="store_true",
        help="If set, single colored pixels are drawn at the calculated center of mass for each island."
    )

    parser.add_argument(
        "-cc", "--center_color",
        type=str,
        default="0,0,0",
        help="The RGB color of the center pixels, e.g., '255,0,0' for red. Requires -c to be set."
    )

    parser.add_argument(
        "-oc", "--only_centers",
        action="store_true",
        help="If set, the script skips island display and outputs ONLY the center pixels on a white background (or transparent if -t is used)."
    )
    
    args = parser.parse_args()
    
    process_image_to_islands(
        args.input_file, 
        args.output_file, 
        args.transparent_black, 
        args.shrink_pixels,
        args.output_yaml,
        args.draw_centers,
        args.center_color,
        args.only_centers
    )

if __name__ == '__main__':
    main()