# portfolio-backend

The backend code for my online portfolio

**Project Sturcture**:

- _Game Backend_: written in c++, using a simple Entity-Comoponents-Systems paradigm. Used as a dynamic library by the python server.

- _Server_: The python server providing APIs using Fast-API and also the sockets for game packets.

- _AI_: Currently a Reinforcment Learning agent, playing as the opponent in the game. The agent is trained with PPO after each game.
