#  Copyright (c). All Rights Reserved.
#  General Optimal control Problem Solver (GOPS)
#  Intelligent Driving Lab (iDLab), Tsinghua University
#
#  Creator: iDLab
#  Lab Leader: Prof. Shengbo Eben Li
#  Email: lisb04@gmail.com
#
#  Description: Noise Function
#  Update Date: 2021-03-10, Yuhang Zhang: Revise Codes


import numpy as np


class EpsilonScheduler:
    """
    Epsilon-greedy scheduler with epsilon schedule.
    """

    def __init__(self, EPS_START=0.9, EPS_END=0.05, EPS_DECAY=2000):
        self.start = EPS_START
        self.end = EPS_END
        self.decay = EPS_DECAY

    def sample(self, action, action_num, steps):
        """Choose an action based on epsilon-greedy policy.

        Args:
            action (any): Predicted action, usually greedy.
            action_num (int): Num of discrete actions.
            steps (int): Global training steps.

        Returns:
            any: Action chosen by psilon-greedy policy.
        """
        thresh = self.end + (self.start - self.end) * np.exp(-steps / self.decay)
        if np.random.random() > thresh:
            return action
        else:
            return np.random.randint(action_num)


class EpsilonGreedy:
    def __init__(self, epsilon, action_num):
        self.epsilon = epsilon
        self.action_num = action_num

    def sample(self, action):
        if np.random.random() > self.epsilon:
            return action
        else:
            return np.array(np.random.randint(self.action_num))


class GaussNoise:
    def __init__(self, mean, std, decay_rate=0.0, min_std=0.0):
        self.mean = mean
        self.std = np.asarray(std, dtype=np.float32)
        self.decay_rate = decay_rate
        self.min_std = np.zeros_like(self.std) + min_std

    def sample(self, action):
        noisy_action = action + np.random.normal(self.mean, self.std)
        self.std = np.maximum(self.min_std, self.std * (1.0 - self.decay_rate))
        return noisy_action


class OrnsteinUhlenbeckNoise:
    def __init__(
        self,
        mean,
        std,
        decay_rate=0.0,
        min_std=0.0,
        mean_attraction_constant=0.15,
        sample_time=1.0,
        initial_action=0.0,
    ):
        self.mean = np.asarray(mean, dtype=np.float32)
        self.std = np.asarray(std, dtype=np.float32)
        self.decay_rate = decay_rate
        self.min_std = np.zeros_like(self.std) + min_std
        self.mean_attraction_constant = mean_attraction_constant
        self.sample_time = sample_time
        self.value = np.zeros_like(self.std) + initial_action

    def sample(self, action):
        self.value = (
            self.value
            + self.mean_attraction_constant
            * (self.mean - self.value)
            * self.sample_time
            + self.std
            * np.random.normal(size=self.std.shape)
            * np.sqrt(self.sample_time)
        )
        self.std = np.maximum(self.min_std, self.std * (1.0 - self.decay_rate))
        return action + self.value
