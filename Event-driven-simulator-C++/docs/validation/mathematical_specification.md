# EMBI Mathematical Specification

This document defines the formal mathematical framework underlying the Estimated Marginal Blocking Impact (EMBI) scheduling algorithm and its evaluation. It serves as the primary reference for all verification tests in Phase 0.5.

## 1. System Model

We model a computer system executing a workload consisting of a set of processes $P = \{p_1, p_2, \dots, p_n\}$.
* Time is discretized into ticks $t \in \mathbb{N}$.
* Each process $p_i$ has a queue of pending jobs. The length of this queue at time $t$ is denoted $q_i(t)$.
* Executing a process for one tick reduces its queue length by 1, i.e., $q_i(t+1) = \max(0, q_i(t) - 1)$.
* A scheduling policy $\pi$ maps a system state at time $t$ to a chosen process $p^*(t)$ to execute.
* The expected total residual waiting time under policy $\pi$ from time $t$ is $J(\pi, t) = \sum_{t'=t}^{\infty} \sum_{i=1}^n q_i(t')$.

## 2. Graph Notation and Adjacency Matrix

Processes are not independent. The completion of jobs in one process may unblock or create jobs in another process. This causal relationship is represented as a directed graph $G = (P, E)$.

* An edge $e = (p_i, p_j) \in E$ indicates that process $p_j$ is causally dependent on process $p_i$.
* This is formalized using an **Adjacency Matrix** $A \in \mathbb{R}^{n \times n}$.
* $A_{ji} = 1$ if $p_j$ depends on $p_i$ (meaning executing $p_i$ helps release or unblock $p_j$).
* $A_{ji} = 0$ otherwise.

## 3. The Oracle EMBI Definition

The true **Marginal Blocking Impact** of choosing process $p_i$ at time $t$ is defined as the reduction in total expected future waiting time compared to a baseline policy.

Let $\pi$ be a baseline scheduling policy (e.g., FCFS).
Let $\pi_i$ be the counterfactual policy that *forces* the execution of process $p_i$ at the current tick $t$, and subsequently follows the baseline policy $\pi$ for all $t' > t$.

The true Oracle EMBI score for process $p_i$ is:
$$ \mathrm{EMBI}(p_i) = J(\pi, t) - J(\pi_i, t) $$

A larger $\mathrm{EMBI}(p_i)$ implies that forcing the execution of $p_i$ now leads to a greater reduction in overall system waiting time.

## 4. The Estimator Definition and Katz Equation

Computing the Oracle EMBI requires full simulation of all possible futures, which is computationally infeasible for online scheduling. Therefore, we introduce an **Estimator** to approximate $\mathrm{EMBI}(p_i)$ using the static structure of the dependency graph $G$.

We define a baseline score vector $\mathbf{b} \in \mathbb{R}^n$, where $b_i$ represents the isolated value of executing process $p_i$ ignoring dependencies (e.g., its queue length $q_i(t)$, or a function of its arrival time).

The EMBI score vector $\mathbf{c} \in \mathbb{R}^n$ propagates these baseline scores upstream through the dependency graph using the **Katz Recurrence**:

$$ \mathbf{c} = \mathbf{b} + \alpha A \mathbf{c} $$

Where:
* $\alpha \in (0, 1)$ is the attenuation factor.
* $A$ is the adjacency matrix defining dependencies.
* $\mathbf{c}$ converges to $(I - \alpha A)^{-1} \mathbf{b}$ provided the spectral radius of $A$ is less than $1/\alpha$.

For a specific process $p_i$, the estimated EMBI score is $c_i$. The scheduler then selects $p^*(t) = \arg\max_i c_i$.

## 5. Counterfactual Replay Definition

To empirically validate the EMBI framework without deploying it in a live kernel, we use **Counterfactual Replay**.

Given a trace of a historical execution $H = \{ (t_k, p^{(hist)}_k, A_k) \}$, a simulated trace replay engine reconstructs the system state at each time $t$.

1. **State Reconstruction**: The simulator parses $H$ to reconstruct $P$, $q_i(t)$, and $A(t)$.
2. **Oracle Evaluation**: At decision points, the simulator clones the system state and runs the `OracleEvaluator` to compute $J(\pi)$ and $J(\pi_i)$ exactly.
3. **Estimator Evaluation**: The `EMBIEstimator` computes $\mathbf{c}$ using only the instantaneous state $A(t)$ and $\mathbf{b}(t)$.
4. **Validation**: We compare $\arg\max_i c_i$ against $\arg\max_i \mathrm{EMBI}(p_i)$ to measure the fidelity of the estimator against the theoretical ground truth.
