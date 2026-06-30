"""DDPG training entry for the ``simu_rl_water`` Simulink environment.

Observation order:
    0. Integrated tracking error, saturated to [-10, 10] in Simulink.
    1. Tracking error: target water level minus measured water level.
    2. Measured water level; an episode terminates outside (0, 20).

Action:
    Physical flow command. The model uses dh/dt = 0.25 * action - 0.1 * sqrt(h).
    [-100, 100] is retained as a safety bound, while the actor directly emits
    physical actions to match the MATLAB actor's linear output layer.

Reward:
    10 when abs(tracking error) < 0.1, otherwise -1. Water-level termination
    adds -100, so the terminal reward is -101 or -90.
"""

import argparse

import numpy as np

from gops.create_pkg.create_alg import create_alg
from gops.create_pkg.create_buffer import create_buffer
from gops.create_pkg.create_env import create_env
from gops.create_pkg.create_evaluator import create_evaluator
from gops.create_pkg.create_sampler import create_sampler
from gops.create_pkg.create_trainer import create_trainer
from gops.utils.init_args import init_args
from gops.utils.plot_evaluation import plot_all
from gops.utils.tensorboard_setup import save_tb_to_csv, start_tensorboard

# Load the native Simulink environment before importing Ray on Windows.
import ray


def parse_bool(value):
    if isinstance(value, bool):
        return value
    value = value.lower()
    if value in {"true", "1", "yes", "on"}:
        return True
    if value in {"false", "0", "no", "off"}:
        return False
    raise argparse.ArgumentTypeError("Expected true or false")


def build_parser():
    parser = argparse.ArgumentParser()

    parser.add_argument("--env_id", default="simu_rl_water")
    parser.add_argument("--algorithm", default="DDPG")
    parser.add_argument("--enable_cuda", action="store_true")
    parser.add_argument("--seed", type=int, default=0)
    parser.add_argument("--is_render", action="store_true")
    parser.add_argument("--is_adversary", action="store_true")
    parser.add_argument("--max_episode_steps", type=int, default=200)
    parser.set_defaults(action_scale=False)

    parser.add_argument("--value_func_name", default="ActionValue")
    parser.add_argument("--value_func_type", default="MLP")
    parser.add_argument("--value_hidden_sizes", default=[25, 25])
    parser.add_argument("--value_hidden_activation", default="relu")
    parser.add_argument("--value_output_activation", default="linear")

    parser.add_argument("--policy_func_name", default="DirectDetermPolicy")
    parser.add_argument("--policy_func_type", default="MLP")
    parser.add_argument("--policy_act_distribution", default="default")
    parser.add_argument("--policy_hidden_sizes", default=[25, 25])
    parser.add_argument("--policy_hidden_activation", default="relu")

    parser.add_argument("--value_learning_rate", type=float, default=1e-3)
    parser.add_argument("--policy_learning_rate", type=float, default=1e-4)
    parser.add_argument("--value_l2_regularization", type=float, default=1e-4)
    parser.add_argument("--policy_l2_regularization", type=float, default=1e-4)
    parser.add_argument("--gamma", type=float, default=1.0)
    parser.add_argument("--tau", type=float, default=1e-3)
    parser.add_argument("--delay_update", type=int, default=1)
    parser.add_argument("--gradient_clip", type=float, default=1.0)

    parser.add_argument("--trainer", default="off_serial_trainer")
    parser.add_argument("--max_episode", type=int, default=2000)
    parser.add_argument(
        "--enable_episode_limit",
        type=parse_bool,
        default= False,
        help="Use max_episode as a stopping condition (true or false)",
    )
    parser.add_argument("--max_iteration", type=int, default=40000)
    parser.add_argument("--ini_network_dir", default=None)
    parser.add_argument("--buffer_name", default="replay_buffer")
    parser.add_argument("--buffer_warm_size", type=int, default=400)
    parser.add_argument("--buffer_max_size", type=int, default=1_000_000)
    parser.add_argument("--replay_batch_size", type=int, default=400)

    parser.add_argument("--sampler_name", default="off_sampler")
    parser.add_argument("--sample_batch_size", type=int, default=8)
    parser.add_argument("--sample_interval", type=int, default=1)
    parser.add_argument("--noise_type", default="ou")
    parser.add_argument("--noise_std_physical", type=float, default=0.3)
    parser.add_argument("--noise_decay_rate", type=float, default=1e-5)

    parser.add_argument("--evaluator_name", default="evaluator")
    parser.add_argument("--num_eval_episode", type=int, default=20)
    parser.add_argument("--eval_interval", type=int, default=500)
    parser.set_defaults(eval_save=True)
    parser.add_argument(
        "--save_folder",
        default=None,
        help="Result directory; defaults to a timestamped GOPS run directory",
    )
    parser.add_argument("--apprfunc_save_interval", type=int, default=300)
    parser.add_argument("--log_save_interval", type=int, default=50)

    return parser


def main():
    args = vars(build_parser().parse_args())
    if not args.pop("enable_episode_limit"):
        args["max_episode"] = None
    args["noise_params"] = {
        "mean": np.array([0.0], dtype=np.float32),
        "std": np.array([args["noise_std_physical"]], dtype=np.float32),
        "decay_rate": args["noise_decay_rate"],
    }
    if args["noise_type"].lower() == "ou":
        args["noise_params"].update(
            mean_attraction_constant=0.15,
            sample_time=1.0,
        )

    env = create_env(**args)
    args = init_args(env, **args)
    env.close()

    start_tensorboard(args["save_folder"])
    algorithm = create_alg(**args)
    algorithm.set_parameters(
        {
            "gamma": args["gamma"],
            "tau": args["tau"],
            "delay_update": args["delay_update"],
            "gradient_clip": args["gradient_clip"],
        }
    )
    sampler = create_sampler(**args)
    buffer = create_buffer(**args)
    evaluator = create_evaluator(**args)
    trainer = create_trainer(algorithm, sampler, buffer, evaluator, **args)

    try:
        trainer.train()
        print("Training is finished!")
    finally:
        ray.shutdown()

    plot_all(args["save_folder"])
    save_tb_to_csv(args["save_folder"])
    print("Results: {}".format(args["save_folder"]))


if __name__ == "__main__":
    main()
