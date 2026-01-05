## sillytui

A frontend for chatting/RP with LLMs, designed to be a TUI version of [SillyTavern](https://github.com/SillyTavern/SillyTavern).

Still in development.

![demo](assets/demo.gif)



### Requirements
- ncurses
- curl

### Installation & Usage

```bash
sudo apt update && sudo apt install -y cmake libncursesw5-dev libcurl4-openssl-dev
git clone https://github.com/AlpinDale/sillytui.git && cd sillytui
make run
```

See `/help` for available commands.


### Tokenization
We have a self-contained tokenization library that supports the following tokenizers:
- tiktoken
- gpt2bpe
- sentencepiece

You can test it like this:

```bash
make example ARGS="--list"  # get a list of available tokenizers
# output:
Available tokenizers:

  openai          OpenAI cl100k (GPT-4, GPT-3.5)
  openai-o200k    OpenAI o200k (GPT-4o)
  qwen3           Qwen 3 (151k vocab)
  llama3          Llama 3 / 3.1 (128k vocab)
  glm4            GLM-4.5 (151k vocab)
  deepseek        DeepSeek R1 (128k vocab)


make example ARGS="-t deepseek 'Hello, world!'"
# output:
Tokenizer: deepseek (DeepSeek R1 (128k vocab))
Text: "Hello, world!"

Token count: 4

Tokens: [19923, 14, 2058, 3]

Decoded tokens:
  [0] 19923 -> "Hello"
  [1] 14 -> ","
  [2] 2058 -> "\xc4\xa0world"
  [3] 3 -> "!"
```


## Acknowledgments

- [Kat](https://github.com/Theldus/kat) for C syntax highlighting
- [ulight](https://github.com/eisenwave/ulight) for general purpose syntax highlighting
