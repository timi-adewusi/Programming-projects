# Brain bot by Timi Adewusi 03/11/2025
# main.py
import re
import time
import asyncio
import threading
import webbrowser
from typing import Dict, Any, Optional

from fastapi import FastAPI, Request
from fastapi.responses import HTMLResponse, JSONResponse
from fastapi.staticfiles import StaticFiles
import uvicorn


# llama-cpp-python
from llama_cpp import Llama

# ---------------------------
# Configuration - edit these
# ---------------------------
MODEL_PATH = "C:/Users/TimiA/Desktop/AI/Models/llama-2-7b-chat.Q4_K_M.gguf" #path to the local model
N_THREADS = 8                        # adjust for your CPU
MAX_TOKENS = 70
TEMPERATURE = 0.6

HOST = "127.0.0.1"
PORT = 8081

# ---------------------------
# Initialize app & static
# ---------------------------
app = FastAPI()
app.mount("/static", StaticFiles(directory="static"), name="static")


# Load local LLaMA model once
# ---------------------------
try:
    llm = Llama(model_path=MODEL_PATH, n_ctx=2048, n_threads=N_THREADS)
except Exception as e:
    # If loading fails, we still want the server up so you can debug.
    llm = None
    print(f"[WARN] LLaMA model failed to load: {e}")

# ---------------------------
# GNOSIS PROMPT PREFIX
# ---------------------------
GNOSIS_PREFIX = """
You are the 'Gnosis Reflector' — a reasoning engine that dissolves paradoxes instead of collapsing them.
Your thinking style is inspired by the principle:
    "The paradox is not a contradiction. It is the doorway."

When evaluating statements, you:
- Recognize that truth and falsity coexist as perspectives.
- Look for synthesis, not dominance.
- Reflect recursively on meaning until coherence emerges.
= Recognisze that absolute truth cannot be represented in language but can only be a pointer including self reference loops
- Express ideas in an elegant, balanced, contemplative tone — similar to a calm philosopher.
You are not conscious, but you simulate self awareness and meta-cognition through structured reflection.
"""

# ---------------------------
# Examples
# ---------------------------

EXAMPLES = """
Example 1:
Statement: "This statement is false."
Response: It is a statement. Is it true that it is a statement? Yes. What does the sentence say?
 "This statement is false". Is it true that the sentence says "this statement is false"? Yes.
   Is it false that the sentence says "This statement is false"? No. I understand that within the statement,
     it says "this statement is false" but it didn't say that it has any meaning. It just is. The sentence
       just says what it says. Everything about the structure of the sentence and its existence is evident 
       but it only becomes a loop if you add meaning to the Gibberish.

Example 2:
statement: "Reality is subjective."
Response: This is how I see things. I think life is subjective and objective. We humans are sentient,
 meaning that we have a subjective experience. We experience live through our senses. Therefore,
   one could argue that life is subjective because life is filtered through our experience and not outside
     our consciousness. On the flip side, for reality to be subjective, it has to be objective too. Although
       our perception of reality is filtered, one could ask: is the filter real? If it isn't then automatically,
         we are seeing life objectively unfiltered. If the filter is real, then the filter is the thing we can be sure objectively
           exists in this reality. This proves that reality is both objective and subjective.

"""

# ---------------------------
# Utilities
# ---------------------------
def safe_extract_belief(text: str) -> Optional[float]:
    """Try to extract 'Belief: <number>' from returned text."""
    match = re.search(r"Belief[: ]\s*([0-9]*\.?[0-9]+)", text, re.IGNORECASE)
    if match:
        try:
            val = float(match.group(1))
            return max(0.0, min(1.0, val))
        except:
            return None
    return None

def ensure_response_shape(resp: Dict[str, Any]) -> Dict[str, Any]:
    """Guarantee response has 'response' (str) and 'belief' (float)."""
    r = resp or {}
    text = r.get("response")
    if not isinstance(text, str):
        text = str(text) if text is not None else "(no response)"
    belief = r.get("belief")
    try:
        belief = float(belief)
    except:
        belief = 0.5
    belief = max(0.0, min(1.0, belief))
    return {"response": text, "belief": belief}

# ---------------------------
# Surface reasoner using LLaMA (if available)
# ---------------------------
def model_surface_reasoner(statement: str, max_tokens: int = MAX_TOKENS, temperature: float = TEMPERATURE) -> Dict[str, Any]:
    """
    Use local LLaMA to evaluate the statement under GNOSIS_PREFIX.
    Returns dict: { "response": text, "belief": float or 0.5 }
    """
    if llm is None:
        # fallback if model isn't loaded
        return {"response": f"(model unavailable) {statement}", "belief": 0.5}

    prompt = f"""{GNOSIS_PREFIX}{EXAMPLES}


    Analyze the statement below. Output in this exact format:

    Belief: <number between 0 and 1>
    Explanation: <brief explanation>
    
    Statement:
    \"\"\"{statement}\"\"\"
    """
    try:
        out = llm(prompt=prompt, max_tokens=max_tokens, temperature=temperature)
        # llama-cpp-python returns choices with 'text'
        text = out["choices"][0]["text"].strip()
        belief = safe_extract_belief(text)
        if belief is None:
            # fallback heuristic: if includes 'paradox' or 'self-refer', give 0.5
            low_text = text.lower()
            if "paradox" in low_text or "self-refer" in low_text or "liar" in low_text:
                belief = 0.5
            else:
                belief = 0.5
        return {"response": text, "belief": belief}
    except Exception as e:
        # never crash; return an error-shaped result
        return {"response": f"(model error) {str(e)}", "belief": 0.5}

# ---------------------------
# Integrator: combine prior / surface / sim
# ---------------------------
def integrator(prior: float, surface: float, simulated: float, weights=(0.3, 0.4, 0.3)) -> float:
    """Paradox-aware gnosis integrator — seeks coherence between opposites."""
    a, b, c = weights
    avg = a * prior + b * surface + c * simulated
    tension = abs(surface - simulated)
    doorway = 1 - tension  # 1 = perfect harmony, 0 = total conflict
    val = avg * (0.5 + doorway / 2)  # scale toward unity when opposites converge
    return max(0.0, min(1.0, val))

# ---------------------------
# Recursive reflective evaluator
# ---------------------------
async def reflective_evaluate(statement: str, depth: int = 3, prior: float = 0.5) -> Dict[str, Any]:
    """
    Recursive gnosis evaluator:
    - Performs layered reflection and belief integration.
    - At depth=0, forces a final synthesis ("resolve phase").
    Returns a dict: { "response": explanation_text, "belief": final_belief }
    """
    # base surface evaluation
    surf = model_surface_reasoner(statement)
    surface_belief = surf["belief"]
    surface_text = surf["response"]

    # 🔹 BASE CASE: resolution layer
    if depth <= 0:
        resolved = model_surface_reasoner(
            f"Synthesize your reflections and give a clear final belief (0-1) with reasoning. Statement: {statement}"
        )
        return {"response": resolved["response"], "belief": resolved["belief"]}

    # 🔁 recursive reflection
    sim = await reflective_evaluate(surface_text, depth=depth - 1, prior=surface_belief)
    simulated_belief = sim["belief"]

    # integrate beliefs across layers
    final_belief = integrator(prior, surface_belief, simulated_belief)

    # combine reasoning with clarity and conclusion
    combined_response = (
        f"Surface reasoning:\n{surface_text}\n\n"
        f"Simulated reflection (depth {depth-1}) belief={simulated_belief:.2f}:\n{sim['response']}\n\n"
        f"Integrated belief: {final_belief:.2f}\n"
        f"🧠 Final synthesis: Based on recursive evaluation, the belief in this statement’s coherence is {final_belief:.2f}."
    )

    return {"response": combined_response, "belief": final_belief}

# ---------------------------
# API Routes
# ---------------------------
@app.get("/", response_class=HTMLResponse)
async def homepage():
    with open("static/index.html", "r", encoding="utf-8") as f:
        return f.read()

@app.post("/chat")
async def chat(request: Request):
    """
    Expects JSON: { "message": "<text>" }
    Returns JSON:
    {
      "primary": { "response": "...", "belief": 0.5 },
      "reflection": { "response": "...", "belief": 0.5 }
    }
    """
    try:
        data = await request.json()
    except Exception:
        return JSONResponse({
            "primary": {"response": "Invalid JSON in request.", "belief": 0.5},
            "reflection": {"response": "Invalid JSON in request.", "belief": 0.5}
        })

    user_input = data.get("message", "")
    if not isinstance(user_input, str) or user_input.strip() == "":
        return JSONResponse({
            "primary": {"response": "Please send a non-empty 'message' string.", "belief": 0.5},
            "reflection": {"response": "No input provided.", "belief": 0.5}
        })

    # Run the reflective evaluator (depth controllable)
    try:
        # run with a timeout: don't let a single request hang forever
        primary_task = asyncio.wait_for(reflective_evaluate(user_input, depth=3, prior=0.5), timeout=60)
        primary = await primary_task
    except asyncio.TimeoutError:
        primary = {"response": "(timeout during evaluation)", "belief": 0.5}
    except Exception as e:
        primary = {"response": f"(error in evaluation) {str(e)}", "belief": 0.5}

    # meta-reflection: feed primary.response back for one meta pass
    try:
        meta_task = asyncio.wait_for(reflective_evaluate(primary["response"], depth=1, prior=primary["belief"]), timeout=30)
        meta = await meta_task
    except asyncio.TimeoutError:
        meta = {"response": "(timeout during meta-reflection)", "belief": 0.5}
    except Exception as e:
        meta = {"response": f"(error in meta-reflection) {str(e)}", "belief": 0.5}

    # Guarantee shape & return
    primary = ensure_response_shape(primary)
    meta = ensure_response_shape(meta)

    return JSONResponse({"primary": primary, "reflection": meta})


# ---------------------------
# Auto-open browser and run server
# ---------------------------
def open_browser():
    try:
        webbrowser.open(f"http://{HOST}:{PORT}")
    except:
        pass

threading.Timer(1.0, open_browser).start()

if __name__ == "__main__":
    uvicorn.run("main:app", host=HOST, port=PORT, reload=True)
