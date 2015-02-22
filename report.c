#include <stddef.h>
#include <assert.h>

#include "mytypes.h"
#include "report.h"
#include "encount.h"
#include "cegt.h"
#include "string.h"
#include "gauss.h"
#include "math.h"

#include "ordolim.h"

#define MINGAMES 1000

//=== duplicated in main.c =========

static ptrdiff_t
head2head_idx_sdev (ptrdiff_t x, ptrdiff_t y)
{	
	ptrdiff_t idx;
	if (y < x) 
		idx = (x*x-x)/2+y;					
	else
		idx = (y*y-y)/2+x;
	return idx;
}

//==================

static void
calc_encounters__
				( int selectivity
				, const struct GAMES *g
				, const bool_t *flagged
				, struct ENCOUNTERS	*e
) 
{
	e->n = 
	calc_encounters	( selectivity
					, g
					, flagged
					, e->enc);

}


static int
compare__ (const player_t *a, const player_t *b, const double *reference )
{	
	const player_t *ja = a;
	const player_t *jb = b;
	const double *r = reference;

	const double da = r[*ja];
	const double db = r[*jb];
    
	return (da < db) - (da > db);
}

static void
insertion_sort (const double *reference, size_t n, player_t *vect)
{
	size_t i, j;
	player_t tmp;
	for (j = n-1; j > 0; j--) {
		for (i = j; i < n; i++) {
			if (0 < compare__(&vect[i-1], &vect[i], reference)) {
				tmp = vect[i-1]; vect[i-1] = vect[i]; vect[i] = tmp; // swap
			}	
		}
	}

}

static size_t
find_maxlen (const char *nm[], size_t n)
{
	size_t maxl = 0;
	size_t length;
	size_t i;
	for (i = 0; i < n; i++) {
		length = strlen(nm[i]);
		if (length > maxl) maxl = length;
	}
	return maxl;
}

#if 0
static bool_t 
is_super_player(size_t j, const struct PLAYERS *pPlayers)
{
	assert(pPlayers->perf_set);
	return pPlayers->performance_type[j] == PERF_SUPERLOSER 
		|| pPlayers->performance_type[j] == PERF_SUPERWINNER
		|| pPlayers->performance_type[j] == PERF_NOGAMES
	;		
}
#endif

#define MAXSYMBOLS_STR 5
static const char *SP_symbolstr[MAXSYMBOLS_STR] = {"<",">","*"," ","X"};

static const char *
get_super_player_symbolstr(size_t j, const struct PLAYERS *pPlayers)
{
	assert(pPlayers->perf_set);
	if (pPlayers->performance_type[j] == PERF_SUPERLOSER) {
		return SP_symbolstr[0];
	} else if (pPlayers->performance_type[j] == PERF_SUPERWINNER) {
		return SP_symbolstr[1];
	} else if (pPlayers->performance_type[j] == PERF_NOGAMES) {
		return SP_symbolstr[2];
	} else if (pPlayers->performance_type[j] == PERF_NORMAL) {
		return SP_symbolstr[3];
	} else
		return SP_symbolstr[4];
}

static bool_t
is_old_version(int32_t j, const struct rel_prior_set *rps)
{
	size_t i;
	bool_t found;
	size_t rn = rps->n;
	const struct relprior *rx = rps->x;
	for (i = 0, found = FALSE; !found && i < rn; i++) {
		found = j == rx[i].player_b;
	}
	return found;
}

static double
rating_round(double x, int d)
{
	const int al[6] = {1,10,100,1000,10000,100000};
	int i;
	double y;
	if (d > 5) d = 5;
	if (d < 0) d = 0;
	y = x * al[d] + 0.5; 
	i = (int) floor(y);
	return (double)i/al[d];
}

static char *
get_sdev_str (double sdev, double confidence_factor, char *str, int decimals)
{
	double x = sdev * confidence_factor;
	if (sdev > 0.00000001) {
		sprintf(str, "%6.*f", decimals, rating_round(x, decimals));
	} else {
		sprintf(str, "%s", "  ----");
	}
	return str;
}

#if 0
static void
flag_lowfrequency_players 	( const struct PLAYERS 	*p
							, const struct RATINGS 	*r
							, gamesnum_t mingames)
{
	size_t j;
	for (j = 0; j < p->n; j++) {
		if (r->playedby_results[j] < mingames)
			p->flagged[j] = TRUE;
	}
}
#endif

static bool_t
ok_to_out (size_t j, const struct output_qualifiers *poutqual, const struct PLAYERS *p, const struct RATINGS *r)
{
	gamesnum_t games = r->playedby_results[j];
	bool_t ok = !p->flagged[j]
				&& games > 0
				&& (!poutqual->mingames_set || games >= poutqual->mingames);
	return ok;
} 

//======================

void 
cegt_output	( const struct GAMES 	*g
			, const struct PLAYERS 	*p
			, const struct RATINGS 	*r
			, struct ENCOUNTERS 	*e  // memory just provided for local calculations
			, double 				*sdev
			, long 					simulate
			, double				confidence_factor
			, const struct GAMESTATS *pgame_stats
			, const struct DEVIATION_ACC *s
			, struct output_qualifiers outqual)
{
	struct CEGT cegt;
	size_t j;

	calc_encounters__(ENCOUNTERS_NOFLAGGED, g, p->flagged, e);
	calc_obtained_playedby(e->enc, e->n, p->n, r->obtained, r->playedby);
	for (j = 0; j < p->n; j++) {
		r->sorted[j] = (int32_t) j; //FIXME size_t
	}

	insertion_sort (r->ratingof_results, p->n, r->sorted);

	cegt.n_enc = e->n; 
	cegt.enc = e->enc;
	cegt.simulate = simulate;
	cegt.n_players = p->n;
	cegt.sorted = r->sorted;
	cegt.ratingof_results = r->ratingof_results;
	cegt.obtained_results = r->obtained_results;
	cegt.playedby_results = r->playedby_results;
	cegt.sdev = sdev; 
	cegt.flagged = p->flagged;
	cegt.name = p->name;
	cegt.confidence_factor = confidence_factor;

	cegt.gstat = pgame_stats;

	cegt.sim = s;

	cegt.outqual = outqual;

	output_cegt_style ("general.dat", "rating.dat", "programs.dat", &cegt);
}


// Function provided to have all head to head information

void 
head2head_output( const struct GAMES 	*g
				, const struct PLAYERS 	*p
				, const struct RATINGS 	*r
				, struct ENCOUNTERS 	*e  // memory just provided for local calculations
				, double 				*sdev
				, long 					simulate
				, double				confidence_factor
				, const struct GAMESTATS *pgame_stats
				, const struct DEVIATION_ACC *s
				, const char *head2head_str)
{
	struct CEGT cegt;
	size_t j;

	calc_encounters__(ENCOUNTERS_NOFLAGGED, g, p->flagged, e);
	calc_obtained_playedby(e->enc, e->n, p->n, r->obtained, r->playedby);
	for (j = 0; j < p->n; j++) {
		r->sorted[j] = (int32_t)j; //FIXME size_t
	}

	insertion_sort (r->ratingof_results, p->n, r->sorted);

	cegt.n_enc = e->n;
	cegt.enc = e->enc;
	cegt.simulate = simulate;
	cegt.n_players = p->n;
	cegt.sorted = r->sorted;
	cegt.ratingof_results = r->ratingof_results;
	cegt.obtained_results = r->obtained_results;
	cegt.playedby_results = r->playedby_results;
	cegt.sdev = sdev; 
	cegt.flagged = p->flagged;
	cegt.name = p->name;
	cegt.confidence_factor = confidence_factor;

	cegt.gstat = pgame_stats;

	cegt.sim = s;

	output_report_individual (head2head_str, &cegt, (int)simulate);
}

#ifndef NDEBUG
static bool_t 
is_empty_player(size_t j, const struct PLAYERS *pPlayers)
{
	assert(pPlayers->perf_set);
	return pPlayers->performance_type[j] == PERF_NOGAMES
	;		
}
#endif

void
all_report 	( const struct GAMES 	*g
			, const struct PLAYERS 	*p
			, const struct RATINGS 	*r
			, const struct rel_prior_set *rps
			, struct ENCOUNTERS 	*e  // memory just provided for local calculations
			, double 				*sdev
			, long 					simulate
			, bool_t				hide_old_ver
			, double				confidence_factor
			, FILE 					*csvf
			, FILE 					*textf
			, double 				white_advantage
			, double 				drawrate_evenmatch
			, int					decimals
			, struct output_qualifiers	outqual)
{
	FILE *f;
	size_t i, j;
	size_t ml;
	char sdev_str_buffer[80];
	const char *sdev_str;

	int rank = 0;
	bool_t showrank = TRUE;

	calc_encounters__(ENCOUNTERS_NOFLAGGED, g, p->flagged, e);

	calc_obtained_playedby(e->enc, e->n, p->n, r->obtained, r->playedby);

	for (j = 0; j < p->n; j++) {
		r->sorted[j] = (int32_t)j; //FIXME size_t
	}

	insertion_sort (r->ratingof_results, p->n, r->sorted);

	/* output in text format */
	f = textf;
	if (f != NULL) {

		ml = find_maxlen (p->name, p->n);
		if (ml > 50) ml = 50;

		if (simulate < 2) {
			fprintf(f, "\n%s %-*s    :%7s %9s %7s %6s\n", 
				"   #", 			
				(int)ml,
				"PLAYER", "RATING", "POINTS", "PLAYED", "(%)");
	
			for (i = 0; i < p->n; i++) {

				j = (size_t)r->sorted[i]; //FIXME size_t

				if (ok_to_out (j, &outqual, p, r)) {

					char rankbuf[80];
					showrank = !is_old_version((int32_t)j, rps); //FIXME size_t
					if (showrank) {
						rank++;
						sprintf(rankbuf,"%d",rank);
					} else {
						rankbuf[0] = '\0';
					}

					if (showrank
						|| !hide_old_ver
					){
						fprintf(f, "%4s %-*s %s :%7.*f %9.1f %7ld %6.1f%s\n", 
							rankbuf,
							(int)ml+1,
							p->name[j],
							get_super_player_symbolstr(j,p),
							decimals,
							rating_round (r->ratingof_results[j], decimals), 
							r->obtained_results[j], 
							(long)r->playedby_results[j], 
							r->playedby_results[j]==0? 0: 100.0*r->obtained_results[j]/(double)r->playedby_results[j], 
							"%"
						);
					}
				} 
			}

		} else {
			fprintf(f, "\n%s %-*s    :%7s %6s %8s %7s %6s\n", 
				"   #", 
				(int)ml, 
				"PLAYER", "RATING", "ERROR", "POINTS", "PLAYED", "(%)");
	
			for (i = 0; i < p->n; i++) {
				j = (size_t) r->sorted[i]; //FIXME size_t

				sdev_str = get_sdev_str (sdev[j], confidence_factor, sdev_str_buffer, decimals);

				assert(r->playedby_results[j] != 0 || is_empty_player(j,p));

				if (ok_to_out (j, &outqual, p, r)) {

					char rankbuf[80];
					showrank = !is_old_version((int32_t)j, rps);
					if (showrank) {
						rank++;
						sprintf(rankbuf,"%d",rank);
					} else {
						rankbuf[0] = '\0';
					}

					if (showrank
						|| !hide_old_ver
					){

						fprintf(f, "%4s %-*s %s :%7.*f %s %8.1f %7ld %6.1f%s\n", 
						rankbuf,
						(int)ml+1, 
						p->name[j],
						get_super_player_symbolstr(j,p),
						decimals,
						rating_round(r->ratingof_results[j], decimals), 
						sdev_str, 
						r->obtained_results[j], 
						(long)r->playedby_results[j], 
						r->playedby_results[j]==0?0:100.0*r->obtained_results[j]/(double)r->playedby_results[j], 
						"%"
						);
					}

				}
#if 0
				 else if (!is_super_player(j,p)) {
					fprintf(f, "%4lu %-*s   :%7.*f %s %8.1f %7ld %6.1f%s\n", 
						i+1,
						(int)ml+1, 
						p->name[j], 
						decimals,
						rating_round(r->ratingof_results[j], decimals), 
						"  ****", 
						r->obtained_results[j], 
						(long)r->playedby_results[j], 
						r->playedby_results[j]==0?0:100.0*r->obtained_results[j]/(double)r->playedby_results[j], 
						"%"
					);
				} else {

					fprintf(f, "%4lu %-*s   :%7s %s %8s %7s %6s%s\n", 
						i+1,
						(int)ml+1,
						p->name[j], 
						"----", "  ----", "----", "----", "----","%"
					);
				}
#endif

			}
		}

		fprintf (f,"\n");
		fprintf (f,"White advantage = %.2f\n",white_advantage);
		fprintf (f,"Draw rate (equal opponents) = %.2f %s\n",100*drawrate_evenmatch, "%");
		fprintf (f,"\n");

	} /*if*/

	/* output in a comma separated value file */
	f = csvf;
	if (f != NULL) {
			fprintf(f, "\"%s\""
			",\"%s\""
			",%s"
			",%s"
			",%s"
			",%s"
			",%s"
			"\n"	
			,"#"	
			,"Player"
			,"\"Rating\"" 
			,"\"Error\"" 
			,"\"Score\""
			,"\"Games\""
			,"\"(%)\"" 
			);
		rank = 0;
		for (i = 0; i < p->n; i++) {
			j = (size_t) r->sorted[i]; //FIXME size_t

			if (ok_to_out (j, &outqual, p, r)) {
				rank++;

				if (sdev[j] > 0.00000001) {
					sprintf(sdev_str_buffer, "%.1f", sdev[j] * confidence_factor);
					sdev_str = sdev_str_buffer;
				} else {
					sdev_str = "\"-\"";
				}

				fprintf(f, "%d,"
				"\"%s\",%.1f"
				",%s"
				",%.2f"
				",%ld"
				",%.2f"
				"\n"		
				,rank
				,p->name[j]
				,r->ratingof_results[j] 
				,sdev_str
				,r->obtained_results[j]
				,(long)r->playedby_results[j]
				,r->playedby_results[j]==0?0:100.0*r->obtained_results[j]/(double)r->playedby_results[j] 
				);
			}
		}
	}

	return;
}



void
errorsout(const struct PLAYERS *p, const struct RATINGS *r, const struct DEVIATION_ACC *s, const char *out, double confidence_factor)
{
	FILE *f;
	ptrdiff_t idx;
	player_t y,x;
	size_t i, j;

	if (NULL != (f = fopen (out, "w"))) {

		fprintf(f, "\"N\",\"NAME\"");	
		for (i = 0; i < p->n; i++) {
			fprintf(f, ",%ld", (long) i);		
		}
		fprintf(f, "\n");	

		for (i = 0; i < p->n; i++) {
			y = r->sorted[i];

			fprintf(f, "%ld,\"%21s\"", (long) i, p->name[y]);

			for (j = 0; j < i; j++) {
				x = r->sorted[j];

				idx = head2head_idx_sdev ((ptrdiff_t)x, (ptrdiff_t)y);

				fprintf(f,",%.1f", s[idx].sdev * confidence_factor);
			}

			fprintf(f, "\n");

		}

		fclose(f);

	} else {
		fprintf(stderr, "Errors with file: %s\n",out);	
	}
	return;
}


void
ctsout(const struct PLAYERS *p, const struct RATINGS *r, const struct DEVIATION_ACC *s, const char *out)
{
	FILE *f;
	ptrdiff_t idx;
	player_t y;
	player_t x;
	size_t i,j;

	if (NULL != (f = fopen (out, "w"))) {

		fprintf(f, "\"N\",\"NAME\"");	
		for (i = 0; i < p->n; i++) {
			fprintf(f, ",%ld",(long) i);		
		}
		fprintf(f, "\n");	

		for (i = 0; i < p->n; i++) {
			y = r->sorted[i];
			fprintf(f, "%ld,\"%21s\"", (long) i, p->name[y]);

			for (j = 0; j < p->n; j++) {
				double ctrs, sd, dr;
				x = r->sorted[j];
				if (x != y) {
					dr = r->ratingof_results[y] - r->ratingof_results[x];
					idx = head2head_idx_sdev ((ptrdiff_t)x, (ptrdiff_t)y);
					sd = s[idx].sdev;
					ctrs = 100*gauss_integral(dr/sd);
					fprintf(f,",%.1f", ctrs);
				} else {
					fprintf(f,",");
				}
			}
			fprintf(f, "\n");
		}
		fclose(f);
	} else {
		fprintf(stderr, "Errors with file: %s\n",out);	
	}
	return;
}
