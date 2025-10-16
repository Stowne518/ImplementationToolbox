#include "class.h"
#include <stdexcept>

void Deck::shuffle() {
	// Simple shuffle algorithm (Fisher-Yates)
	for (int i = maxSize - 1; i > 0; --i) {
		int j = rand() % (i + 1);
		std::swap(cards[i], cards[j]);
	}
}

Card Deck::dealCard()
{
	if (topIndex < 0) throw std::out_of_range("No cards left in deck.");
	return cards[topIndex--];
}