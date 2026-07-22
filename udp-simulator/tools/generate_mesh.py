import os
import sys
import random
from collections import defaultdict, deque
from jinja2 import Template

# CONFIGURATION
TEMPLATE_PATH = '../docker/docker-compose.yml.j2'
OUTPUT_PATH = '../docker/docker-compose.yml'
WARNING_LIMIT = 50
#

def get_user_input(prompt_text):
    while True:
        try:
            count = int(input(prompt_text))
            if count <= 0:
                print("Please enter a number greater than 0.")
                continue
            return count
        except ValueError:
            print("Invalid input. Please enter an integer.")

def build_mesh():
    num_relays = get_user_input("How many relays do you want to generate? ")
    
    if num_relays > WARNING_LIMIT:
        proceed = input(f"WARNING: Generating {num_relays} relays might consume significant system resources. Proceed? [y/n]: ").strip().lower()
        if proceed != 'y':
            print("Aborting generation.")
            sys.exit(0)
            
    num_exits = get_user_input("How many exit nodes do you want? ")

    #! Too many?
    # Create a large enough pool of potential networks
    TOTAL_NETWORKS = num_relays * 2
    
    relays = []
    active_networks = {1}
    all_generated_networks = set()
    
    # Dynamic ratio: 1 extra interface for every 10 relays (minimum of 2)
    max_extra_interfaces = max(2, num_relays // 10)

    for i in range(1, num_relays + 1):
        nets = set()
        
        # 80% chance to be part of the main mesh, 20% chance to be a stranded island
        is_main_mesh = random.random() < 0.8
        num_extra = random.randint(1, max_extra_interfaces)
        
        if is_main_mesh:
            # Anchor to the active mesh to guarantee reachability
            nets.add(random.choice(list(active_networks)))
            
            # Sample strictly from inactive networks
            inactive_pool = set(range(2, TOTAL_NETWORKS + 1)) - active_networks
            if inactive_pool:
                chosen_extras = random.sample(list(inactive_pool), min(num_extra, len(inactive_pool)))
                nets.update(chosen_extras)
                
            active_networks.update(nets)
        else:
            # Stranded island node (only picks from networks not currently active)
            inactive_pool = set(range(2, TOTAL_NETWORKS + 1)) - active_networks
            if len(inactive_pool) < num_extra + 1:
                inactive_pool = set(range(2, TOTAL_NETWORKS + 1)) # Fallback
            
            if inactive_pool:
                nets = set(random.sample(list(inactive_pool), min(num_extra + 1, len(inactive_pool))))

        all_generated_networks.update(nets)

        relays.append({
            "name": f"relay_{i}",
            "networks": [f"zone_{n}" for n in nets]
        })

    # Build adjacency list to map which networks are bridged together
    graph = defaultdict(set)
    for relay in relays:
        relay_nets = [int(n.split('_')[1]) for n in relay["networks"]]
        for n1 in relay_nets:
            for n2 in relay_nets:
                if n1 != n2:
                    graph[n1].add(n2)

    # Breadth-First Search (BFS) to find the true depth of each network from zone_1
    depths = {1: 0}
    queue = deque([1])
    
    while queue:
        current = queue.popleft()
        for neighbor in graph[current]:
            if neighbor not in depths:
                #? Shouldn't we save the minimum between the current value and depths[current]+1 if it IS in the depths dict?
                depths[neighbor] = depths[current] + 1
                queue.append(neighbor)

    # Sort reachable networks by depth descending (furthest networks first)
    reachable_networks = sorted(depths.keys(), key=lambda x: depths[x], reverse=True)
    
    # Place exit nodes
    exit_nodes = []
    
    # Grab the furthest N networks (or all of them if num_exits > reachable_networks)
    target_networks = reachable_networks[:num_exits] if reachable_networks else [1]
    
    for i in range(1, num_exits + 1):
        # If user wants 10 exits but we only have 3 networks, loop back through the 3 networks safely
        target_net = target_networks[(i - 1) % len(target_networks)]
        exit_nodes.append({
            "name": f"exit_{i}",
            "networks": [f"zone_{target_net}"]
        })
        all_generated_networks.add(target_net)

    # Format the final list of networks that actually have nodes attached
    final_networks = [f"zone_{n}" for n in sorted(list(all_generated_networks))]

    script_dir = os.path.dirname(os.path.abspath(__file__))
    template_file = os.path.join(script_dir, TEMPLATE_PATH)
    output_file = os.path.join(script_dir, OUTPUT_PATH)

    try:
        with open(template_file, 'r') as f:
            template = Template(f.read())
            
        rendered_yaml = template.render(
            relays=relays, 
            exit_nodes=exit_nodes,
            networks=final_networks
        )
        
        with open(output_file, 'w') as f:
            f.write(rendered_yaml)
            
        print(f"Success! Generated mesh with {num_relays} relays and {num_exits} exits.")
        print(f"Total active subnets created: {len(final_networks)}")
        print(f"Maximum network depth from victim: {depths.get(reachable_networks[0], 0) if reachable_networks else 0} hops")
        
    except FileNotFoundError:
        print(f"Error: Could not find the template at {template_file}")

if __name__ == "__main__":
    build_mesh()