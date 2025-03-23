from fastapi import FastAPI, HTTPException
from pydantic import BaseModel
import torch
import torch.nn as nn
import torch.optim as optim
import numpy as np
from collections import deque
import logging

# Logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

# --- Connect Four Logic ---
class ConnectFour:
    def __init__(self):
        self.rows = 6
        self.cols = 7
        self.board = np.zeros((self.rows, self.cols), dtype=int)

    def reset(self):
        self.board = np.zeros((self.rows, self.cols), dtype=int)

    def to_list(self):
        return self.board.flatten().tolist()

    def from_list(self, board_list):
        if len(board_list) != 42 or not all(x in [0, 1, 2] for x in board_list):
            raise ValueError("Invalid board")
        self.board = np.array(board_list).reshape(self.rows, self.cols)

    def drop_piece(self, col, player):
        if col < 0 or col >= self.cols or self.board[0, col] != 0:
            return False
        for row in range(self.rows - 1, -1, -1):
            if self.board[row, col] == 0:
                self.board[row, col] = player
                return True
        return False

    def check_winner(self):
        # Horizontal
        for r in range(self.rows):
            for c in range(self.cols - 3):
                if self.board[r, c] != 0 and all(self.board[r, c:c+4] == self.board[r, c]):
                    return self.board[r, c]
        # Vertical
        for r in range(self.rows - 3):
            for c in range(self.cols):
                if self.board[r, c] != 0 and all(self.board[r:r+4, c] == self.board[r, c]):
                    return self.board[r, c]
        # Positive diagonal
        for r in range(self.rows - 3):
            for c in range(self.cols - 3):
                if self.board[r, c] != 0 and all(self.board[r+i, c+i] == self.board[r, c] for i in range(4)):
                    return self.board[r, c]
        # Negative diagonal
        for r in range(3, self.rows):
            for c in range(self.cols - 3):
                if self.board[r, c] != 0 and all(self.board[r-i, c+i] == self.board[r, c] for i in range(4)):
                    return self.board[r, c]
        # Draw
        if np.all(self.board != 0):
            return 0
        return -1  # Ongoing

# --- PPO Agent ---
class PolicyNetwork(nn.Module):
    def __init__(self, state_dim, action_dim):
        super().__init__()
        self.net = nn.Sequential(
            nn.Linear(state_dim, 256),
            nn.ReLU(),
            nn.Linear(256, 128),
            nn.ReLU(),
            nn.Linear(128, action_dim)
        )

    def forward(self, state):
        return self.net(state)

class ValueNetwork(nn.Module):
    def __init__(self, state_dim):
        super().__init__()
        self.net = nn.Sequential(
            nn.Linear(state_dim, 256),
            nn.ReLU(),
            nn.Linear(256, 128),
            nn.ReLU(),
            nn.Linear(128, 1)
        )

    def forward(self, state):
        return self.net(state)

class PPOAgent:
    def __init__(self):
        self.state_dim = 42  # 6x7 board
        self.action_dim = 7  # 7 columns
        self.policy = PolicyNetwork(self.state_dim, self.action_dim)
        self.value = ValueNetwork(self.state_dim)
        self.optimizer_policy = optim.Adam(self.policy.parameters(), lr=0.001)
        self.optimizer_value = optim.Adam(self.value.parameters(), lr=0.001)
        self.gamma = 0.99
        self.eps_clip = 0.2
        self.memory = deque(maxlen=1000)  # (state, action, reward, next_state, prob)

    def get_action(self, state, game):
        state_tensor = torch.tensor(state, dtype=torch.float32)
        logits = self.policy(state_tensor)
        # Mask invalid actions (full columns)
        available = [c for c in range(7) if game.board[0, c] == 0]
        if not available:
            return None, None
        masked_logits = logits[available]
        probs = torch.softmax(masked_logits, dim=-1)
        action_dist = torch.distributions.Categorical(probs)
        action_idx = action_dist.sample()
        action = available[action_idx]
        log_prob = action_dist.log_prob(action_idx)
        return action, log_prob

    def compute_advantage(self, reward, state, next_state):
        state_value = self.value(torch.tensor(state, dtype=torch.float32))
        next_value = self.value(torch.tensor(next_state, dtype=torch.float32))
        advantage = reward + self.gamma * next_value - state_value
        return advantage.detach()

    def update(self):
        if len(self.memory) < 5:
            return

        states, actions, rewards, next_states, old_probs = zip(*self.memory)
        states = torch.tensor(states, dtype=torch.float32)
        actions = torch.tensor(actions, dtype=torch.long)
        rewards = torch.tensor(rewards, dtype=torch.float32)
        next_states = torch.tensor(next_states, dtype=torch.float32)
        old_probs = torch.tensor(old_probs, dtype=torch.float32)

        # Compute advantages
        advantages = [self.compute_advantage(r, s, ns) for r, s, ns in zip(rewards, states, next_states)]
        advantages = torch.stack(advantages).squeeze()

        # Policy update
        for _ in range(3):
            logits = self.policy(states)
            game = ConnectFour()
            available_masks = []
            for s in states:
                game.from_list(s.tolist())
                mask = [1 if game.board[0, c] == 0 else 0 for c in range(7)]
                available_masks.append(mask)
            masked_logits = logits * torch.tensor(available_masks, dtype=torch.float32) + (1 - torch.tensor(available_masks, dtype=torch.float32)) * -1e10
            probs = torch.softmax(masked_logits, dim=-1)
            dist = torch.distributions.Categorical(probs)
            new_probs = dist.log_prob(actions)
            ratio = torch.exp(new_probs - old_probs)
            surr1 = ratio * advantages
            surr2 = torch.clamp(ratio, 1 - self.eps_clip, 1 + self.eps_clip) * advantages
            policy_loss = -torch.min(surr1, surr2).mean()

            self.optimizer_policy.zero_grad()
            policy_loss.backward()
            self.optimizer_policy.step()

        # Value update
        values = self.value(states).squeeze()
        value_loss = nn.MSELoss()(values, rewards + self.gamma * self.value(next_states).squeeze())
        self.optimizer_value.zero_grad()
        value_loss.backward()
        self.optimizer_value.step()

        self.memory.clear()
        logger.info("PPO policy updated")
#
# # Initialize game and agent
# game = ConnectFour()
# agent = PPOAgent()
