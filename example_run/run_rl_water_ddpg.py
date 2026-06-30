"""Load a GOPS DDPG checkpoint and run one ``simu_rl_water`` episode."""

import argparse
import json
import pathlib

import matplotlib.pyplot as plt
import numpy as np
import torch

from gops.create_pkg.create_alg import create_approx_contrainer
from gops.create_pkg.create_env import create_env


ACTION_LOW = -100.0
ACTION_HIGH = 100.0


def build_parser():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--log_dir",
        default="results/simu_rl_water/DDPG_smoke",
        help="Training result directory containing config.json",
    )
    parser.add_argument("--iteration", type=int, default=5)
    parser.add_argument("--seed", type=int, default=0)
    return parser


def main():
    cli_args = build_parser().parse_args()
    log_dir = pathlib.Path(cli_args.log_dir).resolve()

    with (log_dir / "config.json").open("r", encoding="utf-8") as file:
        args = json.load(file)

    args.update(
        vector_env_num=None,
        reward_scale=None,
        repeat_num=None,
        gym2gymnasium=False,
    )
    env = create_env(**args)
    networks = create_approx_contrainer(**args)

    checkpoint = log_dir / "apprfunc" / "apprfunc_{}.pkl".format(
        cli_args.iteration
    )
    state_dict = torch.load(str(checkpoint), map_location="cpu")
    networks.load_state_dict(state_dict)
    networks.eval()

    observations = []
    actions = []
    rewards = []
    physical_actions = []

    observation, info = env.reset(seed=cli_args.seed)
    try:
        for _ in range(args.get("max_episode_steps", 200)):
            observations.append(observation.copy())
            observation_tensor = torch.from_numpy(
                observation.astype(np.float32)
            ).unsqueeze(0)
            with torch.no_grad():
                logits = networks.policy(observation_tensor)
                action = networks.create_action_distributions(logits).mode()
            action = action.cpu().numpy()[0]

            normalized_low = env.action_space.low
            normalized_high = env.action_space.high
            physical_action = ACTION_LOW + (ACTION_HIGH - ACTION_LOW) * (
                (action - normalized_low) / (normalized_high - normalized_low)
            )

            observation, reward, done, info = env.step(action)
            actions.append(action.copy())
            physical_actions.append(physical_action.copy())
            rewards.append(float(reward))
            if done:
                break
    finally:
        env.close()

    output_dir = log_dir / "evaluator"
    output_dir.mkdir(parents=True, exist_ok=True)
    output_path = output_dir / "manual_iter{}_seed{}.npz".format(
        cli_args.iteration, cli_args.seed
    )
    np.savez(
        str(output_path),
        observations=np.asarray(observations),
        actions=np.asarray(actions),
        physical_actions=np.asarray(physical_actions),
        rewards=np.asarray(rewards),
    )

    observations_array = np.asarray(observations)
    physical_actions_array = np.asarray(physical_actions).reshape(-1)
    rewards_array = np.asarray(rewards)
    reference_height = observations_array[:, 1] + observations_array[:, 2]

    figure, axes = plt.subplots(3, 1, figsize=(8, 8), sharex=True)
    axes[0].plot(observations_array[:, 2], label="water level")
    axes[0].plot(reference_height, "--", label="reference")
    axes[0].set_ylabel("height")
    axes[0].legend()
    axes[0].grid(True)
    axes[1].plot(physical_actions_array)
    axes[1].set_ylabel("physical action")
    axes[1].grid(True)
    axes[2].plot(rewards_array)
    axes[2].set_ylabel("reward")
    axes[2].set_xlabel("step")
    axes[2].grid(True)
    figure.tight_layout()
    figure_path = output_path.with_suffix(".png")
    figure.savefig(str(figure_path), dpi=150)
    plt.close(figure)

    print("Checkpoint: {}".format(checkpoint))
    print("Episode steps: {}".format(len(rewards)))
    print("Episode return: {}".format(sum(rewards)))
    print("First normalized action: {}".format(actions[0]))
    print("First physical action: {}".format(physical_actions[0]))
    print("Final observation: {}".format(observation))
    print("TimeLimit.truncated: {}".format(info.get("TimeLimit.truncated", False)))
    print("Saved trajectory: {}".format(output_path))
    print("Saved figure: {}".format(figure_path))


if __name__ == "__main__":
    main()
