/*
 * Dynamic namespace metrics, PMDA helper routines.
 *
 * Copyright (c) 2013-2014 Red Hat.
 * Copyright (c) 2010 Aconex.  All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 */

#include "pmapi.h"
#include "impl.h"
#include "pmda.h"

static struct dynamic {
    const char		*prefix;
    int			prefixlen;
    int			mtabcount;	/* internal use only */
    int			extratrees;	/* internal use only */
    //int			nclusters;
    //int			*clusters;
    pmdaUpdatePMNS	pmnsupdate;
    pmdaUpdateText	textupdate;
    pmdaUpdateMetric	mtabupdate;
    pmdaCountMetrics	mtabcounts;
    pmdaNameSpace	*pmns;
    pmdaMetric		*metrics;	/* original fixed table */
    int			nmetrics;	/* fixed metrics number */
    unsigned int	clustermask;	/* mask out parts of PMID cluster */
} *dynamic;

static int dynamic_count;

static inline unsigned int
dynamic_pmid_cluster(int index, pmID pmid)
{
    if (dynamic[index].clustermask)
	return pmid_cluster(pmid) & dynamic[index].clustermask;
    return pmid_cluster(pmid);
}

int
pmdaDynamicSetClusterMask(const char *prefix, unsigned int mask)
{
    int i;

    for (i = 0; i < dynamic_count; i++) {
	if (strcmp(prefix, dynamic[i].prefix) == 0)
	    continue;
	dynamic[i].clustermask = mask;
	return 0;
    }
    return -EINVAL;
}

void
pmdaDynamicPMNS(const char *prefix,
	    int *clusters, int nclusters,
	    pmdaUpdatePMNS pmnsupdate, pmdaUpdateText textupdate,
	    pmdaUpdateMetric mtabupdate, pmdaCountMetrics mtabcounts,
	    pmdaMetric *metrics, int nmetrics)
{
    int size = (dynamic_count+1) * sizeof(struct dynamic);
    int *ctab;
    size_t ctabsz;

    if ((dynamic = (struct dynamic *)realloc(dynamic, size)) == NULL) {
	__pmNotifyErr(LOG_ERR, "out-of-memory registering dynamic metrics");
	return;
    }
    //ctabsz = sizeof(int) * nclusters;
    //if ((ctab = (int *)malloc(ctabsz)) == NULL) {
//	__pmNotifyErr(LOG_ERR, "out-of-memory registering dynamic clusters");
//	free(dynamic);
//	return;
 //   }

    //fprintf(stderr, "Adding %s at location %d\n", prefix, dynamic_count);

    dynamic[dynamic_count].prefix = prefix;
    dynamic[dynamic_count].prefixlen = strlen(prefix);
    //dynamic[dynamic_count].nclusters = nclusters;
    //dynamic[dynamic_count].clusters = ctab;
    //memcpy(dynamic[dynamic_count].clusters, clusters, ctabsz);
    dynamic[dynamic_count].pmnsupdate = pmnsupdate;
    dynamic[dynamic_count].textupdate = textupdate;
    dynamic[dynamic_count].mtabupdate = mtabupdate;
    dynamic[dynamic_count].mtabcounts = mtabcounts;
    dynamic[dynamic_count].pmns = NULL;
    dynamic[dynamic_count].metrics = metrics;
    dynamic[dynamic_count].nmetrics = nmetrics;
    dynamic[dynamic_count].clustermask = 0;
    dynamic_count++;
}

/*
 * Verify if a given pmid should be handled by this dynamic instance.
 * Assumes pmnsupdate has been called so dynamic[index].pmns can't be NULL
 */

int
pmdaDynamicCheckPMID( pmID pmid, int index )
{

    int numfound=0;
    char **nameset;

    pmdaNameSpace *tree = dynamic[index].pmns;
    //__pmDumpNameNode(stderr, tree->root, 1);
    numfound = pmdaTreeName(tree, pmid, &nameset);

    int domain = pmid_domain(pmid);
    int cluster = pmid_cluster(pmid);
    int item = pmid_item(pmid);

    //fprintf(stderr, "Check: %d.%d.%d In: %d Result: %d\n", domain, cluster, item, index, numfound);

    /* Don't need the names, just seeing if its there */
    
    if( numfound > 0 ){
	free(nameset);
	return numfound;
    }

    return 0;

}

/*
 *  * Verify if a given name should be handled by this dynamic instance.
 *   * Assumes pmnsupdate has been called so dynamic[index].pmns can't be NULL
 *    */

int
pmdaDynamicCheckName( char *name, int index )
{

    int found = 0;
    pmID pmid;

    pmdaNameSpace *tree = dynamic[index].pmns;

    /* Use this because we want non-leaf nodes also, since we are called for child checks */
    if( __pmdaNodeLookup(tree->root->first, name) != NULL ){
	//fprintf( stderr, "Ours[%d]: %s\n", index, name);
	return 1;
    }
    else{
	//fprintf( stderr, "Not Ours[%d]: %s\n", index, name);
	return 0;
    }

}

pmdaNameSpace *
pmdaDynamicLookupName(pmdaExt *pmda, const char *name)
{
    int i,sts;

    for (i = 0; i < dynamic_count; i++) {
	sts = dynamic[i].pmnsupdate(pmda, &dynamic[i].pmns);
	if (sts)
	    pmdaDynamicMetricTable(pmda);
	if( pmdaDynamicCheckName( name, i )){
	    //fprintf(stderr, "pmdaDynamicLookupName found %s in %d\n", name, i);
	    return dynamic[i].pmns;
	}
    }
    return NULL;
}

pmdaNameSpace *
pmdaDynamicLookupPMID(pmdaExt *pmda, pmID pmid)
{
    int i, j, cluster, sts;

    for (i = 0; i < dynamic_count; i++) {
		/* Need to do this in all cases to be able to search the pmns */
		sts = dynamic[i].pmnsupdate(pmda, &dynamic[i].pmns);
		if(sts)
		    pmdaDynamicMetricTable(pmda);
		if( pmdaDynamicCheckPMID( pmid, i ) ){
		    return dynamic[i].pmns;
		}
    }
    return NULL;
}

int
pmdaDynamicLookupText(pmID pmid, int type, char **buf, pmdaExt *pmda)
{
    int i, j;

    for (i = 0; i < dynamic_count; i++) {
		/* Need to do this in all cases to be able to search the pmns */
                dynamic[i].pmnsupdate(pmda, &dynamic[i].pmns);
                if( pmdaDynamicCheckPMID( pmid, i ) ){
		    return dynamic[i].textupdate(pmda, pmid, type, buf);
		}
    }
    return -ENOENT;
}

/*
 * Call the update function for each new metric we're adding.
 * We pass in the original metric, and the new (uninit'd) slot
 * which needs to be filled in.  All a bit obscure, really.
 */
static pmdaMetric *
dynamic_metric_table(int index, pmdaMetric *offset, pmdaExt *pmda)
{
    struct dynamic *dp = &dynamic[index];
    int m, tree_count = dp->extratrees;
    int sts=0;

    for (m = 0; m < dp->nmetrics; m++) {
	pmdaMetric *mp = &dp->metrics[m];
	int cluster = dynamic_pmid_cluster(index, mp->m_desc.pmid);
	int c, gid;

		/* Need to do this in all cases to be able to search the pmns */
                sts = dp->pmnsupdate(pmda, &dp->pmns);
		//fprintf(stderr, "Will check %p\n", (__pmID_int *)&mp->m_desc.pmid);
                if( pmdaDynamicCheckPMID( mp->m_desc.pmid, index ) )
		    for (gid = 0; gid < tree_count; gid++)
			dp->mtabupdate(mp, offset++, gid + 1);
    }
    return offset;
}

/*
 * Iterate through the dynamic table working out how many additional metric
 * table entries are needed.  Then allocate a new metric table, if needed,
 * and run through the dynamic table once again to fill in the additional
 * entries.  Finally, we update the metric table pointer within the pmdaExt
 * for libpcp_pmda callback routines subsequent use.
 */
void
pmdaDynamicMetricTable(pmdaExt *pmda)
{
    int i, trees, total, resize = 0;
    pmdaMetric *mtab, *fixed, *offset;

    for (i = 0; i < dynamic_count; i++)
	dynamic[i].mtabcount = dynamic[i].extratrees = 0;

    for (i = 0; i < dynamic_count; i++) {
	dynamic[i].mtabcounts(&total, &trees);
	dynamic[i].mtabcount += total;
	dynamic[i].extratrees += trees;
	resize += (total * trees);
    }

    fixed = dynamic[0].metrics;		/* fixed metrics */
    total = dynamic[0].nmetrics;	/* and the count */

    if (resize == 0) {
	/* Fits into the default metric table - reset it to original values */
fallback:
	if (pmda->e_metrics != fixed)
	    free(pmda->e_metrics);
	pmdaRehash(pmda, fixed, total);
    } else {
	resize += total;
	if ((mtab = calloc(resize, sizeof(pmdaMetric))) == NULL)
	    goto fallback;
	memcpy(mtab, fixed, total * sizeof(pmdaMetric));
	offset = mtab + total;
	for (i = 0; i < dynamic_count; i++)
	    offset = dynamic_metric_table(i, offset, pmda);
	if (pmda->e_metrics != fixed)
	    free(pmda->e_metrics);
	pmdaRehash(pmda, mtab, resize);
    }
}
