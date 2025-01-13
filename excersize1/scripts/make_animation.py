import os
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation

# Ρυθμίσεις directories
SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))
PROJECT_DIR = os.path.abspath(os.path.join(SCRIPT_DIR, ".."))
DATA_DIR = os.path.join(PROJECT_DIR, "data")
GRAPHS_DIR = os.path.join(DATA_DIR, "graphs")

# Δημιουργία φακέλου για γραφήματα αν δεν υπάρχει
os.makedirs(GRAPHS_DIR, exist_ok=True)

# Output path for the animation
OUTPUT_VIDEO = os.path.join(GRAPHS_DIR, "game_of_life_animation.mp4")

def create_test_generations():
    """Creates a small test case with a few generations."""
    # Define a 5x5 grid where the initial state is some cells alive
    initial = np.array([
        [0, 1, 0, 0, 0],
        [0, 1, 0, 0, 0],
        [0, 1, 0, 0, 0],
        [0, 0, 0, 0, 0],
        [0, 0, 0, 0, 0]
    ])

    # Simulate a few generations for testing
    # Generation 1: The "blinker" moves
    generation_1 = np.array([
        [0, 0, 0, 0, 0],
        [1, 1, 1, 0, 0],
        [0, 0, 0, 0, 0],
        [0, 0, 0, 0, 0],
        [0, 0, 0, 0, 0]
    ])

    # Generation 2: The "blinker" continues its oscillation
    generation_2 = np.array([
        [0, 0, 0, 0, 0],
        [0, 1, 0, 0, 0],
        [0, 1, 0, 0, 0],
        [0, 1, 0, 0, 0],
        [0, 0, 0, 0, 0]
    ])

    return [initial, generation_1, generation_2]

def create_animation(generations, output_path, interval=500):
    """Creates an animation of the Game of Life generations."""
    fig, ax = plt.subplots()
    ax.axis("off")  # Hide the axis for better visual clarity
    grid_size = len(generations[0])  # Assume all grids are square
    im = ax.imshow(generations[0], cmap="binary", interpolation="nearest")

    # Add a print statement to check if the image data is being correctly set
    print("Initial Grid:")
    print(generations[0])

    def update(frame):
        """Update the image for each frame."""
        im.set_array(generations[frame])
        print(f"Updating frame {frame}")
        return [im]

    ani = animation.FuncAnimation(
        fig, update, frames=len(generations), interval=interval, blit=True
    )

    # Try saving the animation as a video (this part might need ffmpeg installed)
    ani.save(output_path, writer="ffmpeg", dpi=200)
    plt.close(fig)  # Close the figure after saving the video
    print(f"Animation saved to {output_path}")

# Main execution
if __name__ == "__main__":
    generations = create_test_generations()
    print(f"Created {len(generations)} generations for the test.")
    create_animation(generations, OUTPUT_VIDEO)
