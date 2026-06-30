import rl_water

model_class = rl_water.rl_water()
model_class.initialize()
model_class.step()
del model_class

raw_env = rl_water.RawEnv()
raw_env.seed()
raw_env.reset()
# raw_env.step(act)



gym_env = rl_water.GymEnv()
gym_env.seed()
gym_env.reset()
# gym_env.step(act)


