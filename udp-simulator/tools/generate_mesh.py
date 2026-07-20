import os
import sys
import random
from jinja2 import Template

# CONFIGURATION
TEMPLATE_PATH = '../docker/docker-compose.yml.j2'
OUTPUT_PATH = '../docker/docker-compose.yml'
WARNING_LIMIT = 50
#

def get_relay_count():
    while True:
        try:
            count = int(input("How many relays do you want to generate? "))
            if count <= 0:
                print("Please enter a number greater than 0.")
                continue
                
            if count > WARNING_LIMIT:
                proceed = input(f"WARNING: Generating {count} relays might consume significant system resources. Proceed? [y/n]: ").strip().lower()
                if proceed != 'y':
                    print("Aborting generation.")
                    sys.exit(0)
            return count
        except ValueError:
            print("Invalid input. Please enter an integer.")

def build_mesh():
    num_relays = get_relay_count()
    num_zones = num_relays + 1
    
    # Random Network Generation
    relays = []
    for i in range(1, num_relays + 1):
        # Pick a random number of interfaces for this node (1 to 3 connections)
        num_interfaces = random.randint(1, 3)

        # For keeping only the UNIQUE connections
        nets = set()
        
        # Keep rolling random zones until we have the required number of UNIQUE connections
        while len(nets) < num_interfaces:
            nets.add(f"zone_{random.randint(1, num_zones)}")
            
        relays.append({
            "name": f"relay_{i}",
            "networks": list(nets)
        })

    script_dir = os.path.dirname(os.path.abspath(__file__))
    template_file = os.path.join(script_dir, TEMPLATE_PATH)
    output_file = os.path.join(script_dir, OUTPUT_PATH)

    try:
        with open(template_file, 'r') as f:
            template = Template(f.read())
            
        rendered_yaml = template.render(relays=relays, num_zones=num_zones)
        
        with open(output_file, 'w') as f:
            f.write(rendered_yaml)
            
        print(f"Success! Generated random mesh for 1 victim and {num_relays} relays.")
        print(f"Total isolated subnets created: {num_zones}")
        
    except FileNotFoundError:
        print(f"Error: Could not find the template at {template_file}")

if __name__ == "__main__":
    build_mesh()