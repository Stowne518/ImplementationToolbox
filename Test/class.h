#pragma once
#include <string>

// Simple blackjack game to practice OOP in C++
class Card
{
private:
	char suit;
	char rank;
public:
	Card();
	Card(char s, char r) : suit(s), rank(r) {}
	~Card() {};
	char getSuit() const { return suit; }
	char getRank() const { return rank; }
};

class Deck
{
private:
	Card* cards;	// Dynamic array of cards
	int topIndex;	// Index of the top card in the deck
	int maxSize;
public:
	Deck(int size = 52) : maxSize(size), topIndex(size - 1) {
		cards = new Card[maxSize];							// Dynamically allocate array of cards
		for (int i = 0; i < maxSize; ++i) {
			char suit = "CDHS"[i / 13];						// C=Clubs, D=Diamonds, H=Hearts, S=Spades
			char rank;
			int rankIndex = i % 13;
			if (rankIndex == 0) rank = 'A';					// 'A' for Ace
			else if (rankIndex < 9) rank = '2' + rankIndex;	// '2' to '9'
			else if (rankIndex == 9) rank = 'T';			// 'T' for 10
			else if (rankIndex == 10) rank = 'J';			// 'J' for Jack
			else if (rankIndex == 11) rank = 'Q';			// 'Q' for Queen
			else rank = 'K';								// 'K' for King
			cards[i] = Card(suit, rank);
		}
	};
	~Deck() {
		delete[] cards; // Free dynamically allocated memory
	};
	void shuffle();
	Card dealCard();
	int cardsRemaining() const { return topIndex + 1; }
	void reset();
};

class Hand
{
private:
	Card* cards;	// Dynamic array of cards in hand
	int cardCount;
	int maxSize;
public:
	Hand(int size = 2) {};
	~Hand() {};
	void addCard(const Card& card);
};

class Dealer
{
private:
	Hand* hand; // Dynamic hand for dealer
public:
	Dealer() {};
	~Dealer() {};
	Hand* getHand() const { return hand; }
	void play(Deck& deck);
};

class Player
{
private:
	std::string name;
	int chips;
	Hand* hand; // Dynamic hand for player
public:
	Player(std::string n, int c = 100) {};
	~Player() {};
	std::string getName() const { return name; }
	int getChips() const { return chips; }
	void winChips(int amount) { chips += amount; }
	void loseChips(int amount) { chips -= amount; if(chips < 0) chips = 0; }
	Hand* getHand() const { return hand; }
	bool isBroke() const { return chips <= 0; }
};

class Game
{
private:
	Deck* deck;			// Dynamic deck for the game
	Dealer* dealer;		// Dynamic dealer
	Player** players;	// Dynamic array of player pointers
	int playerCount;
	int maxPlayers;
public:
	Game(int maxP = 4) {};
	~Game() {};
	void addPlayer(const std::string& name);
	void removePlayer(const std::string& name);
	void startRound();
	void endRound();
	void playerHit(const std::string& name);
	void playerStand(const std::string& name);
	void displayHands() const;
	void displayChips() const;
	bool allPlayersBroke() const;
};