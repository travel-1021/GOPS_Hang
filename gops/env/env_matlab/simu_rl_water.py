#  Copyright (c). All Rights Reserved.
#  General Optimal control Problem Solver (GOPS)
#  Intelligent Driving Lab (iDLab), Tsinghua University

"""Simulink water-tank reinforcement-learning environment."""

from typing import Any, Optional

import gym
import numpy as np

from gops.env.env_matlab.resources.simu_rl_water import rl_water


class SimuRlWater(gym.Env):
    """GOPS adapter for the slxpy-generated ``rl_water`` environment."""

    metadata = {"render_modes": []}

    def __init__(self, **kwargs: Any):
        max_episode_steps = kwargs.get("max_episode_steps", 200)
        spec = rl_water._env.EnvSpec(
            id="SimuRlWater-v0",
            max_episode_steps=max_episode_steps,
            strict_reset=True,
        )
        self.env = rl_water.GymEnv(spec)
        self.observation_space = gym.spaces.Box(
            low=self.env.observation_space.low,
            high=self.env.observation_space.high,
            dtype=self.env.observation_space.dtype,
        )
        self.action_space = gym.spaces.Box(
            low=-100.0,
            high=100.0,
            shape=(1,),
            dtype=np.float64,
        )
        self.reward_range = self.env.reward_range

    def reset(
        self,
        *,
        seed: Optional[int] = None,
        options: Optional[dict] = None,
    ):
        return self.env.reset(seed=seed, options=options)

    def step(self, action: np.ndarray):
        action = np.asarray(action, dtype=np.float64)
        obs, reward, terminated, truncated, info = self.env.step(action)
        if truncated:
            info["TimeLimit.truncated"] = True
        return obs, reward, terminated or truncated, info

    def render(self):
        return self.env.render()

    def close(self):
        self.env.close()


def env_creator(**kwargs: Any):
    return SimuRlWater(**kwargs)
