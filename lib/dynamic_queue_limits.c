#include <linux/module.h>
#include <linux/types.h>
#include <linux/ctype.h>
#include <linux/kernel.h>
#include <linux/jiffies.h> 
#include <linux/dynamic_queue_limits.h>
#define POSDIFF(A, B) ((int)((A) - (B)) > 0 ? (A) - (B) : 0)
#define AFTER_EQ(A, B) ((int)((A) - (B)) >= 0)

/* Records completed count and recalculates the queue limit */
void dql_completed(struct dql *dql, unsigned int count)
{
	unsigned int inprogress, prev_inprogress, limit;
	unsigned int ovlimit, completed, num_queued;
	bool all_prev_completed;

	num_queued = ACCESS_ONCE(dql->num_queued);

	/* Can't complete more than what's in queue */
	BUG_ON(count > num_queued - dql->num_completed);

	completed = dql->num_completed + count;
	limit = dql->limit;
	ovlimit = POSDIFF(num_queued - dql->num_completed, limit);
	inprogress = num_queued - completed;
	prev_inprogress = dql->prev_num_queued - dql->num_completed;
	all_prev_completed = AFTER_EQ(completed, dql->prev_num_queued);

	if ((ovlimit && !inprogress) ||
	    (dql->prev_ovlimit && all_prev_completed)) {
		/*
		 * Queue considered starved if:
		 *   - The queue was over-limit in the last interval,
		 *     and there is no more data in the queue.
		 *  OR
		 *   - The queue was over-limit in the previous interval and
		 *     when enqueuing it was possible that all queued data
		 *     had been consumed.  This covers the case when queue
		 *     may have becomes starved between completion processing
		 *     running and next time enqueue was sche
