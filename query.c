// query.c ... query scan functions
// part of Multi-attribute Linear-hashed Files
// Manage creating and using Query objects
// Last modified by John Shepherd, July 2019

#include "defs.h"
#include "query.h"
#include "reln.h"
#include "tuple.h"
#include "chvec.h"
#include "hash.h"
#include "bits.h"
#include "page.h"

// A suggestion ... you can change however you like

struct QueryRep {
	Reln    rel;       // need to remember Relation info
	Bits    known;     // the hash value from MAH
	// Bits    unknown;   // the unknown bits from MAH
	Bits    unknown[32];  // unknown bits, from low to high
	int last_visited;
	int current_visit;
	int new_round_flag;
	// for example, if ...1*1*01, then
	//     it stores [2, 4, ...]
	//     store the last of -1 to represent end
	// need to initialized to all -1
	int 	valid_unknown;// valid length of unknown
	int 	max_valid_unknown;
	int 	current_round;// current round of unknown, up to pow(2, len(valid unknown))
	int 	next_round_flag;
	PageID  curpage;   // current page in scan
	int     is_ovflow; // are we in the overflow pages?
	Offset  curtup;    // offset of current tuple within page
	int 	tuple_num_sum;
	int 	tuple_num_cur;
	int 	finish_flag;
	char*	target_tuple;
	int* 	visited;
	//TODO
};

// take a query string (e.g. "1234,?,abc,?")
// set up a QueryRep object for the scan


Query startQuery(Reln r, char *q)
{
	Query new = malloc(sizeof(struct QueryRep));
	assert(new != NULL);
	// TODO
	// Partial algorithm:
	// form known bits from known attributes
	// form unknown bits from '?' attributes
	// compute PageID of first page
	//   using known bits and first "unknown" value
	// set all values in QueryRep object

	new->rel = r;
	new->target_tuple = malloc((strlen(q)+1) * sizeof(char));
	strcpy(new->target_tuple, q);
	new->target_tuple[strlen(q)] = '\0';
	// printf("%s\n", new->target_tuple);
	for (int i = 0; i < 32; i++) {
		new->unknown[i] = -1;
	}
	Bits temp_unknown[32];
	new->valid_unknown = -1;
	new->current_round = 0;
	new->last_visited = -1;
	new->current_visit = -1;
	new->new_round_flag = -1;

	// traverse through the char sequence
	new->known = 0;
	int char_index = 0;
	int cnt_num = 0;
	char* pos = q;
	char **vals = malloc(nattrs(r) * sizeof(char*));
	Bits *hash_ptr = malloc(nattrs(r) * sizeof(Bits));
	while (*(q + char_index) != '\0') {
		if (*(q + char_index) == ',') {
			*(q + char_index) = '\0';
			*(vals + cnt_num) = pos;
			if (strcmp(*(vals + cnt_num), "?") != 0) {
				*(hash_ptr + cnt_num) = hash_any((unsigned char*)vals[cnt_num], strlen(vals[cnt_num]));
			}
			pos = q + char_index + 1;
			cnt_num++;
		}
		char_index++;
	}
	*(vals + cnt_num) = pos;
	if (strcmp(*(vals + cnt_num), "?") != 0) {
		*(hash_ptr + cnt_num) = hash_any((unsigned char*)vals[cnt_num], strlen(vals[cnt_num]));
	}
	cnt_num++;

	// generate the known and unknown values
	int current = 0;
	ChVecItem* choice_vector = chvec(r);
	for (int i = 0; i < MAXCHVEC; i++) {
		int attribute_index = (choice_vector+i)->att;
		int bit_index = (choice_vector+i)->bit;
		if (strcmp(*(vals+attribute_index), "?") != 0) {
			// append to known
			if (((*(hash_ptr+attribute_index)) & (1<<bit_index)) != 0) {
				new->known += self_pow(i);
				// printf("%d ", i);
			}
		} else {
			// append to unknown
			temp_unknown[current] = i;
			current++;
		}
	}
	// printf("\n");
	// printf("Nattrs: %d\n", nattrs(r));
	// printf("hash index 4: %d\n", *(hash_ptr+4));
	new->valid_unknown = current;
	
	for (int index = current-1; index >= 0; index--) {
		new->unknown[current-1-index] = temp_unknown[index];
	}
	new->max_valid_unknown = 0;
	for (int i = 0; i < new->valid_unknown; i++) {
		if (new->unknown[i] < depth(new->rel) + 1) {
			new->max_valid_unknown++;
		}
	}
	new->visited = malloc(self_pow(new->max_valid_unknown) * sizeof(int));
	for (int i = 0; i < self_pow(new->max_valid_unknown); i++) {
		new->visited[i] = -1;
	}

	// set other fields
	new->curpage = -1;
	new->is_ovflow = 1;
	new->curtup = -1;
	new->tuple_num_sum = -1;
	new->tuple_num_cur = 0;
	new->next_round_flag = 0;// 0 means next round
	new->finish_flag = -1;

	// testing
	// printf("known: %d\n", new->known);
	// printf("unknown:\n");
	// for (int i = 0; i < 32; i++) {
	// 	printf("%d ", new->unknown[i]);
	// }
	// printf("\n");
	// printf("Valid unknown: %d\n", new->valid_unknown);
	// printf("vals:\n");
	// for (int i = 0; i < nattrs(r); i++) {
	// 	printf("%s\n", *(vals+i));
	// }
	// good

	return new;
}


Bits calculate_unknown_value(Bits* unknown, int valid_unknown, Bits current_round) {
	Bits res = 0;
	for (int i = valid_unknown-1; i>=0; i--) {
		if (current_round >= self_pow(i)) {
			res += self_pow(*(unknown + valid_unknown - 1 - i));
			current_round -= self_pow(i);
		}
	}
	return res;
}


// get next tuple during a scan

Tuple raw_getNextTuple(Query q)
{
	// TODO
	// Partial algorithm:
	// if (more tuples in current page)
	//    get next matching tuple from current page
	// else if (current page has overflow)
	//    move to overflow page
	//    grab first matching tuple from page
	// else
	//    move to "next" bucket
	//    grab first matching tuple from data page
	// endif
	// if (current page has no matching tuples)
	//    go to next page (try again)
	// endif
	HOLA:
	if (q->finish_flag == 0) {
		return NULL;
	}
	if (q->next_round_flag == 0) {
		// printf("next_round_flag\n");
		Bits real_hash = q->known | calculate_unknown_value(q->unknown, q->valid_unknown, q->current_round);
		// something wrong when ?,?,?,?,?
		// printf("--------\nreal_hash: %d\n", real_hash);
		// printf("valid_unknown: %d\n", q->valid_unknown);
		// printf("current_round: %d\n", q->current_round);
		// printf("depth: %d\n", depth(q->rel));
		// printf("max_valid_unknown: %d\n", q->max_valid_unknown);
		Bits p;
		q->new_round_flag = 0;
		q->current_visit = -1;
		if (depth(q->rel) == 0) {
			p = 0;
		} else {
			p = getLower(real_hash, depth(q->rel));
			if (p < splitp(q->rel)) p = getLower(real_hash, depth(q->rel)+1);
		}
		// for (int i = 0; i < self_pow(q->max_valid_unknown); i++) {
		// 	if (q->visited[i] == -1) {
		// 		q->visited[i] = p;
		// 		break;
		// 	} 
		// 	else if (q->visited[i] == p) {
		// 		return NULL;
		// 	}
		// 	printf("%d ", q->visited[i]);
		// }
		// printf("\n");
		q->last_visited = p;
		// int real_valid_unknown = q->valid_unknown > depth(q->rel) + 1 ? depth(q->rel) + 1 : q->valid_unknown;
		Page pg = getPage(dataFile(q->rel), p);
		// printf("Searching page %d\n", p);
		q->curpage = p;
		q->is_ovflow = 1;//not
		q->tuple_num_sum = pg->ntuples;
		q->tuple_num_cur = 0;
		q->curtup = 0;
		q->next_round_flag = 1;// not

		char* c = pageData(pg) + q->curtup;
		// Tuple res = malloc((strlen(c)+1)*sizeof(char));
		// strcpy(res, c);
		Tuple res = c;
		q->tuple_num_cur++;
		q->curtup += strlen(c) + 1;
		if (q->tuple_num_cur == q->tuple_num_sum) {
			// if there is overflow page, use the overflow page
			// else turn to the next round
			if (pageOvflow(pg) != NO_PAGE) {
				q->is_ovflow = 0;
				q->curpage = pageOvflow(pg);
				q->curtup = 0;
				q->tuple_num_cur = 0;
			} else {
				q->current_round++;
				q->is_ovflow = 1;
				if (q->current_round == self_pow(q->max_valid_unknown)) {
					// done
					q->finish_flag = 0;
				}
				q->next_round_flag = 0;
			}
		}
		// if (strcmp(res, q->target_tuple)==0) { // something wrong here
		// 	printf("Match!    %s    %s\n", res, q->target_tuple);
		// 	return res;
		// } else {
		// 	printf("Not match!    %s    %s\n", res, q->target_tuple);
		// 	getNextTuple(q);
		// }
		return res;
	} else {
		// printf("next\n");
		// read the same page
		q->new_round_flag = 1;
		Page pg;
		if (q->is_ovflow == 0) {
			pg = getPage(ovflowFile(q->rel), q->curpage);
		} else {
			pg = getPage(dataFile(q->rel), q->curpage);
		}
		// pg = getPage(dataFile(q->rel), q->curpage);
		// if (q->is_ovflow == 0) {
			// printf("*******Want to jump to ov %d\n",q->curpage);
		// }
		// file may be empty!!!
		if (pg->ntuples == 0) {
			if (pageOvflow(pg) != NO_PAGE) {
				q->is_ovflow = 0;
				q->curpage = pageOvflow(pg);
				// printf("*****Change to page %d\n", q->curpage);
				q->curtup = 0;
				q->tuple_num_cur = 0;
			} else {
				q->current_round++;
				q->is_ovflow = 1;
				if (q->current_round == self_pow(q->max_valid_unknown)) {
					q->finish_flag = 0;
					// printf("hola\n");
				}
				q->next_round_flag = 0;
				q->curtup = 0;
				q->tuple_num_cur = 0;
			}
			goto HOLA;
		} else {
			char* c = pageData(pg) + q->curtup;
			// int real_valid_unknown = q->valid_unknown > depth(q->rel) + 1 ? depth(q->rel) + 1 : q->valid_unknown;
			// Tuple res = malloc((strlen(c)+1)*sizeof(char));
			// strcpy(res, c);
			Tuple res = c;
			q->tuple_num_cur++;
			q->curtup += strlen(c) + 1;
			q->tuple_num_sum = pg->ntuples;
			if (q->tuple_num_cur == q->tuple_num_sum) {
				if (pageOvflow(pg) != NO_PAGE) {
					q->is_ovflow = 0;
					q->curpage = pageOvflow(pg);
					// printf("*****Change to page %d\n", q->curpage);
					q->curtup = 0;
					q->tuple_num_cur = 0;
				} else {
					q->current_round++;
					q->is_ovflow = 1;
					if (q->current_round == self_pow(q->max_valid_unknown)) {
						q->finish_flag = 0;
						// printf("hola\n");
					}
					q->next_round_flag = 0;
					q->curtup = 0;
					q->tuple_num_cur = 0;
				}
			}
			// printf("***%s    %ld\n", res, strlen(res));
			// printf("---%s    %ld\n", q->target_tuple, strlen(res));
			// if (strcmp(res, q->target_tuple) == 0) {  // not working here
			// 	printf("Match!    %s    %s\n", res, q->target_tuple);
			// 	return res;
			// }
			// else {
			// 	printf("Not match!    %s    %s\n", res, q->target_tuple);
			// 	getNextTuple(q);
			// }
			return res;
		}
		
			
	}
	return NULL;
}

// int compare_tuples(Tuple a, char* target) {
// 	// return 0 if matches, 1 otherwise

// }

void show_current_page(Query q, PageID n) {
	Page pg = getPage(ovflowFile(q->rel), n);
	Count ntups = pageNTuples(pg);
	char *c = pageData(pg);
	for (int i = 0; i < ntups; i++) {
		printf("**%s\n", c);
		c += strlen(c) + 1;
	}
	PageID ovp = pageOvflow(pg);
	// printf("***%d\n", ovp);
	while (ovp != NO_PAGE) {
		Page ovpg = getPage(ovflowFile(q->rel), ovp);
		Count ntups = pageNTuples(ovpg);
		char *c = pageData(ovpg);
		for (int i = 0; i < ntups; i++) {
			printf("**%s\n", c);
			c += strlen(c) + 1;
		}
		ovp = pageOvflow(ovpg);
		free(ovpg);
	}
}

Tuple getNextTuple(Query q) {
	// printf("------**********--------------\n");
	// show_current_page(q, 12);
	// printf("------**********--------------\n");
	while (1) {
		Tuple t = raw_getNextTuple(q);
		if (t == NULL) {
			return t;
		}
		int pass_flag = -1;
		for (int i = 0; i < self_pow(q->max_valid_unknown); i++) {
			if (q->visited[i] == -1) {
				q->visited[i] = q->last_visited;
				q->current_visit = 0;
				pass_flag = 0;
				break;
			} else if (q->visited[i] == q->last_visited && q->current_visit != 0) {
				pass_flag = -1;
				break;
			} else if (q->visited[i] == q->last_visited && q->current_visit == 0) {
				pass_flag = 0;
				break;
			}
		}
		if (pass_flag == 0) {
			if (tupleMatch(q->rel, t, q->target_tuple)) {
				return t;
			}
		}
	}
}

// last element
// compare attributes

// clean up a QueryRep object and associated data

void closeQuery(Query q)
{
	free(q->target_tuple);
	free(q->visited);
	free(q);
}
