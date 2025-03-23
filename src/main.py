from fastapi import FastAPI, HTTPException
from fastapi.middleware.cors import CORSMiddleware  # Add this
from pydantic import BaseModel

import logging

from RL.RLModules import ConnectFour, PPOAgent

app = FastAPI(title="Connect Four PPO Opponent")

# Add CORS middleware
app.add_middleware(
    CORSMiddleware,
    allow_origins=["http://localhost:8080", "https://username.github.io"],  # Update with your frontend URL(s)
    allow_credentials=True,
    allow_methods=["*"],  # Allow all methods (GET, POST, OPTIONS, etc.)
    allow_headers=["*"],  # Allow all headers
)


# Logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

# Initialize game and agent
game = ConnectFour()
agent = PPOAgent()

# --- API Endpoints ---
class MoveInput(BaseModel):
    board: list[int]  # 42-element list
    column: int       # User's move (0-6)

@app.get("/")
async def root():
    return {"message": "Welcome to my portfolio."}

@app.post("/play")
async def play_game(input: MoveInput):
    global game
    try:
        # Load board state
        game.from_list(input.board)
        col = input.column
        if not 0 <= col <= 6:
            raise HTTPException(status_code=400, detail="Column must be 0-6")

        # User's move (Red = 1)
        if not game.drop_piece(col, 1):
            raise HTTPException(status_code=400, detail="Invalid move: column full")
        prev_state = game.to_list()

        # Check if user won
        winner = game.check_winner()
        if winner != -1:
            reward = -1 if winner == 1 else 0  # User wins or draw
            return {"board": game.to_list(), "winner": winner, "message": "Game over"}

        # Agent's move (Yellow = 2)
        action, log_prob = agent.get_action(prev_state, game)
        if action is None:
            raise HTTPException(status_code=400, detail="No valid moves left")
        game.drop_piece(action, 2)

        # Check result
        winner = game.check_winner()
        reward = 1 if winner == 2 else -1 if winner == 1 else 0 if winner == 0 else 0
        new_state = game.to_list()
        agent.memory.append((prev_state, action, reward, new_state, log_prob.item()))

        # Update policy if game ends
        if winner != -1:
            agent.update()
            game.reset()  # Reset for next game

        return {
            "board": new_state,
            "agent_move": action,
            "winner": winner,
            "message": "Agent moved"
        }
    except Exception as e:
        logger.error(f"Error: {str(e)}")
        raise HTTPException(status_code=500, detail=str(e))

@app.get("/reset")
async def reset_game():
    global game, agent
    game.reset()
    agent = PPOAgent()
    return {"board": game.to_list(), "message": "Game and agent reset"}

# Run with: uvicorn main:app --reload