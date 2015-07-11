#include "jpeg_dec.h"
#include <stdlib.h>
#include <limits.h>

void createNode(node_t **node, int h, int next[], jfif_huff *huf);
void printCodes(node_t *node, char buf[], int height);

// TODO(pmalani): Move this into the huffman table struct?
static int codes_left;

uint8_t *huffPtr;
uint8_t bitsLeft;
uint8_t curByte;

/*
 * Begin the creation of the Huffman table for string decoding
 */
void genHuff(jfif_huff *huf)
{
	// We need an auxiliary array, which tells what's the index of
	// the next code value, for a particular length.
	int i;
	int next[16] = {0};
	int index = 0;
	int h;
	char buf[16] = "";

	for (i = 0; i < 16; i++) {
		if (huf->l[i] == 0) {
			next[i] = -1;
		} else {
			next[i] = index;
			index += huf->l[i];
		}
	}

	// Used to keep track of whether we need to add more nodes to the tree.
	codes_left = index;
	LOGD("Starting to generate Huffman Table\n");

	for (i = 0; i < 16; i++)
		LOGD("Next code index for length %d : %d\n", i + 1, next[i]);

	createNode(&huf->root, 0, next, huf);
	printCodes(huf->root, buf, 0);
}

void printCodes(node_t *node, char buf[], int height) {
	if (!node)
		return;

	if (node->l == NULL && node->r == NULL) {
		buf[height] = '\0';
		LOGD("Bin string = %s, val = %u\n", buf, node->val);
		return;
	}

	buf[height] = '0';
	printCodes(node->l, buf, height + 1);

	buf[height] = '1';
	printCodes(node->r, buf, height + 1);
}

/*
 * Allocate space for a node and initialize the members.
 */
static node_t *allocNode()
{
	node_t *node = malloc(sizeof(node_t));
	node->l = NULL;
	node->r = NULL;
	return node;
}

/*
 * Recursive call to generate the tree.
 * This tree will simply be walked through  when determining the code
 * corresponding to a bit-string.
 */
void createNode(node_t **node, int h, int next[], jfif_huff *huf)
{

	// Special case for root node
	if (h == 0) {
		*node = allocNode();
		createNode(&((*node)->l), h + 1, next, huf);
		createNode(&((*node)->r), h + 1, next, huf);
		return;
	}

	if (h > 16 || (huf->l[h - 1] <= 0 && huf->l[h] <= 0 &&
		!codes_left)) {
		// The greatest bit length is 16, so this is the end codition.
		// The other truncation option is when we have run out codes.
		// This is known when the current height has no more codes,
		// and the next number of codes is 0.
		// TODO(pmalani): Can we have lengths with no codes, followed
		// by lengths with some codes ?
		return;
	}

	*node = allocNode();

	// One offset it for C indexing
	if (huf->l[h - 1] > 0) {
		// Assign a code-val to this node.
		(*node)->val = huf->codes[next[h - 1]];
		next[h - 1] += 1;
		huf->l[h - 1] -= 1;
		codes_left--;
	} else {
		// Continue for left and right
		createNode(&((*node)->l), h + 1, next, huf);
		createNode(&((*node)->r), h + 1, next, huf);
	}
	return;
}

/*
 * FUNCTION initHuffParsing
 *
 * Setup the book keeping while parsing the scan data for huffman codes
 * via the trees. Since this is done on a bit by bit basis, the code here needs
 * to keep track of when the next byte needs to be loaded in, and those bits
 * parsed.
 */
void initHuffParsing(uint8_t *ptr)
{
	huffPtr = ptr;
	bitsLeft = CHAR_BIT;
	curByte = *huffPtr;
}

static inline uint8_t getBit()
{
	uint8_t ret_val;
	if (bitsLeft == 1) {
		ret_val = curByte;
		huffPtr++;
		curByte =*huffPtr;
		bitsLeft = CHAR_BIT;
	} else {
		ret_val = (curByte >> (bitsLeft - 1)) & 0x1;
		bitsLeft--;
	}
	return (ret_val & 0x1);
}

uint8_t traverseTree(node_t *node)
{
	if (!node->l && !node->r)
		return node->val;

	return traverseTree(getBit() ? node->r : node->l);
}

/*
 * FUNCTION getVal
 *
 * Use the bits to traverse the supplied huffman table, and get the next
 * value based on the encoded bit string
 *
 * NOTE: Repeated calls to getVal() will give the successive codes in
 * the bit stream.
 * */
uint8_t getVal(jfif_huff *huff)
{
	return traverseTree(huff->root);
}

/*
 * FUNCTION getNumBits
 *
 * Get the number formed by reading the next successive bits in the data.
 */
uint16_t getNumBits(uint8_t num)
{
	uint16_t ret_val = 0;
	while (num--) {
		ret_val <<= 1;
		ret_val |= getBit();
	}
	return ret_val;
}
