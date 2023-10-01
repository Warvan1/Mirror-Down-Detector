# DownDetector bot

## Dependencies

Install D++ using instructions on the D++ Documentation Website.
[Installation Instructions](https://dpp.dev/installing.html)

There are many ways to install I personally built D++ from source from the github.
[D++ Github](https://github.com/brainboxdotcc/DPP)

## Development ENV File

Create a .env file with the following 3 lines of text for registering slash commands in a single server for testing.

```bash
DiscordToken
test
guild_id
```

## Production ENV File

Create a .env file with the following 2 lines of text for registering slash commands globally for any server the bot joins.

```bash
DiscordToken
global
```

### Start the bot

To start the bot just run the start-bot.sh bash script.