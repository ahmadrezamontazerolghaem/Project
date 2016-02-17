void timerqueue_add(struct timerqueue_head *head, struct timerqueue_node *node)
{
	struct rb_node **p = &head->head.rb_node;
	struct rb_node *parent = NULL;
	struct timerqueue_node  *ptr;

	/* Make sure we don't add nodes that are already added */
	WARN_ON_ONCE(!RB_EMPTY_NODE(&node->node));

	while (*p) {
		parent = *p;
		ptr = rb_entry(parent, struct timerqueue_node, node);
		if (node->expires.tv64 < ptr->expires.tv64)
			p = &(*p)->rb_left;
		else
			p = &(*p)->rb_right;
	}
	rb_link_node(&node->node, parent, p);
	rb_insert_color(&node->node, &head->head);

	if (!head->next || node->expires.tv64 < head->next->expires.tv64)
		head->next = node;
}
EXPORT_SYMBOL_GPL(timerqueue_add);

/**
 * timerqueue_del - Removes a timer from the timerqueue.
 *
 * @head: head of timerqueue
 * @node: timer node to be removed
 *
 * Removes the timer node from the timerqueue.
 */
void timerqueue_del(struct timerqueue_head *head, struct timerqueue_node *node)
{
	WARN_ON_ONCE(RB_EMPTY_NODE(&node->node));

	/* update next pointer */
	if (head->next == node) {
		struct rb_node *rbn = rb_next(&node->node);

		head->next = rbn ?
			rb_entry(rbn, struct timerqueue_node, node) : NULL;
	}
	rb_erase(&node->node, &head->head);
	RB_CLEAR_NODE(&node->node);
}
EXPORT_SYMBOL_GPL(timerqueue_del);

/**
 * timerqueue_iterate_next - Returns the timer after the provided timer
 *
 * @node: Pointer to a timer.
 *
 * Provides the timer that is after the given node. This is used, when
 * necessary, to iterate through the list of timers in a timer list
 * without modifying the list.
 */
struct timerqueue_node *timerqueue_iterate_next(struct timerqueue_node *node)
{
	struct rb_node *next;

	if (!node)
		return NULL;
	next = rb_next(&node->node);
	if (!next)
		return NULL;
	return container_of(next, struct timerqueue_node, node);
}
EXPORT_SYMBOL_GPL(timerqueue_iterate_next);
