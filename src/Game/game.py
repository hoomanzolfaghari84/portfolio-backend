import random
import time
import math
from collections import defaultdict

# --- Components ---
class Health:
    def __init__(self, hp=10):
        self.hp = hp

class Kinematics:
    def __init__(self, x, y, vx=0, vy=0, speed=5):
        self.x = x
        self.y = y
        self.vx = vx
        self.vy = vy
        self.speed = speed

class Combat:
    def __init__(self, cooldown=0.5):
        self.cooldown = cooldown
        self.last_shot = 0

class Armor:
    def __init__(self, value=5):
        self.value = value
        self.is_blocking = False

# --- Entities ---
class Entity:
    def __init__(self, entity_id):
        self.id = entity_id
        self.components = {}

    def add_component(self, component):
        self.components[type(component).__name__] = component

    def get_component(self, component_type):
        return self.components.get(component_type)

class Player(Entity):
    def __init__(self, entity_id, x, y):
        super().__init__(entity_id)
        self.add_component(Health())
        self.add_component(Kinematics(x, y))
        self.add_component(Combat())
        self.add_component(Armor())

class Obstacle(Entity):
    def __init__(self, entity_id, x, y, width, height):
        super().__init__(entity_id)
        self.add_component(Kinematics(x, y, 0, 0, 0))  # Static
        self.width = width
        self.height = height

class Bullet(Entity):
    def __init__(self, entity_id, x, y, vx, vy):
        super().__init__(entity_id)
        self.add_component(Kinematics(x, y, vx, vy, 10))  # Bullet speed

# --- Systems ---
class KinematicsSystem:
    def __init__(self, arena_size):
        self.arena_size = arena_size

    def update(self, entities):
        for eid, entity in list(entities.items()):
            kin = entity.get_component("Kinematics")
            if not kin:
                continue
            kin.x += kin.vx
            kin.y += kin.vy
            kin.x = max(0, min(kin.x, self.arena_size - 20))
            kin.y = max(0, min(kin.y, self.arena_size - 20))

            if isinstance(entity, (Player, Bullet)):
                for oid, obstacle in entities.items():
                    if oid != eid and isinstance(obstacle, Obstacle):
                        o_kin = obstacle.get_component("Kinematics")
                        if (kin.x < o_kin.x + obstacle.width and
                            kin.x + 20 > o_kin.x and
                            kin.y < o_kin.y + obstacle.height and
                            kin.y + 20 > o_kin.y):
                            if isinstance(entity, Bullet):
                                del entities[eid]
                            elif isinstance(entity, Player):
                                kin.x -= kin.vx
                                kin.y -= kin.vy
                                kin.vx = 0
                                kin.vy = 0

class CombatSystem:
    def update(self, entities, current_time):
        for bid, bullet in list(entities.items()):
            if not isinstance(bullet, Bullet):
                continue
            b_kin = bullet.get_component("Kinematics")
            for pid, player in entities.items():
                if isinstance(player, Player) and pid != bullet.id:
                    p_kin = player.get_component("Kinematics")
                    p_health = player.get_component("Health")
                    p_armor = player.get_component("Armor")
                    if (abs(b_kin.x - p_kin.x) < 20 and abs(b_kin.y - p_kin.y) < 20):
                        if p_armor.is_blocking and p_armor.value > 0:
                            p_armor.value -= 1
                        else:
                            p_health.hp -= 1
                        del entities[bid]
                        break

class ActionSystem:
    def __init__(self, arena_size):
        self.arena_size = arena_size
        self.bullet_counter = 0

    def process(self, entities, entity_id, action, current_time):
        entity = entities.get(entity_id)
        if not entity or not isinstance(entity, Player):
            return

        kin = entity.get_component("Kinematics")
        combat = entity.get_component("Combat")
        armor = entity.get_component("Armor")

        if action["type"] == "move":
            direction = action["direction"]
            if direction == "up": kin.vy = -kin.speed
            elif direction == "down": kin.vy = kin.speed
            elif direction == "left": kin.vx = -kin.speed
            elif direction == "right": kin.vx = kin.speed
            elif direction == "stop_x": kin.vx = 0
            elif direction == "stop_y": kin.vy = 0
        elif action["type"] == "shoot" and current_time - combat.last_shot >= combat.cooldown:
            combat.last_shot = current_time
            bullet_id = f"bullet_{self.bullet_counter}"
            self.bullet_counter += 1
            angle = action["angle"]
            vx = 10 * math.cos(angle)
            vy = 10 * math.sin(angle)
            entities[bullet_id] = Bullet(bullet_id, kin.x + 10, kin.y + 10, vx, vy)
        elif action["type"] == "block":
            armor.is_blocking = action["active"]

# --- Game Engine ---
class GameEngine:
    def __init__(self):
        self.arena_size = 400
        self.entities = {}
        self.players = set()
        self.is_running = False
        self.tick_rate = 1 / 60  # 60 FPS
        self.kinematics = KinematicsSystem(self.arena_size)
        self.combat = CombatSystem()
        self.action = ActionSystem(self.arena_size)

    def start(self, player1_id, player2_id):
        """Start the game with two player IDs."""
        self.players = {player1_id, player2_id}
        self.entities.clear()
        self.is_running = True
        # Add players
        self.entities[player1_id] = Player(player1_id, 50, 150)
        self.entities[player2_id] = Player(player2_id, 350, 150)
        # Add random obstacles
        for i in range(5):
            x = random.randint(50, 350)
            y = random.randint(50, 350)
            w = random.randint(20, 50)
            h = random.randint(20, 50)
            self.entities[f"obs_{i}"] = Obstacle(f"obs_{i}", x, y, w, h)

    def end(self):
        """End the game."""
        self.is_running = False
        self.players.clear()
        self.entities.clear()

    def update(self):
        """Update game state for one tick."""
        if not self.is_running:
            return
        current_time = time.time()
        self.kinematics.update(self.entities)
        self.combat.update(self.entities, current_time)

    def process_input(self, player_id, action):
        """Process an input action from a player."""
        if player_id not in self.players or not self.is_running:
            return
        self.action.process(self.entities, player_id, action, time.time())

    def get_state(self):
        """Return current game state for UI/network."""
        state = {
            "entities": {},
            "game_over": any(e.get_component("Health").hp <= 0
                           for e in self.entities.values() if isinstance(e, Player)),
            "is_running": self.is_running,
            "players": list(self.players)
        }
        for eid, e in self.entities.items():
            kin = e.get_component("Kinematics")
            health = e.get_component("Health")
            armor = e.get_component("Armor")
            state["entities"][eid] = {
                "type": "player" if isinstance(e, Player) else "obstacle" if isinstance(e, Obstacle) else "bullet",
                "x": kin.x,
                "y": kin.y,
                "hp": health.hp if health else 0,
                "armor": armor.value if armor else 0,
                "is_blocking": armor.is_blocking if armor else False,
                "width": getattr(e, "width", 20),
                "height": getattr(e, "height", 20)
            }
        return state

    def is_game_over(self):
        """Check if the game is over."""
        return any(e.get_component("Health").hp <= 0
                  for e in self.entities.values() if isinstance(e, Player))